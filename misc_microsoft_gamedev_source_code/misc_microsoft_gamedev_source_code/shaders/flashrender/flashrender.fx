#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"
#include "flashhelpers.inc"

sampler gTex0Sampler : register(s0);
sampler gTex1Sampler : register(s1);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 gMatrixV0;
float4 gMatrixV1;
void flashGlyphVS(in  float4 InPos        : POSITION,
                  in  float2 InTexCoord0  : TEXCOORD0,
                  in  float4 InColor0     : COLOR0,
                  out float4 OutPos       : POSITION,
                  out float2 OutTexCoord0 : TEXCOORD0,
                  out float4 OutColor0    : COLOR0)
{
   OutPos = InPos;
   OutPos.x = dot(InPos, gMatrixV0);
   OutPos.y = dot(InPos, gMatrixV1);
   OutTexCoord0 = InTexCoord0;
   OutColor0 = InColor0.bgra;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 gCxForm0;
float4 gCxForm1;
void flashGlyphPS(in  float2 InTexCoord0 : TEXCOORD0,
                  in  float4 InColor0    : COLOR0,
                  out float4 OutColor0   : COLOR0,
                  uniform bool bPremultipliedColor)
{   
   InColor0.a = InColor0.a * tex2D(gTex0Sampler, InTexCoord0).a;
   OutColor0 = InColor0 * gCxForm0 + gCxForm1;

   if (bPremultipliedColor)
   {
      OutColor0.rgb = OutColor0.rgb * OutColor0.a;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void flashUberGlyphVS(in   int    Index        : INDEX,
                      out  float4 OutPosition  : POSITION,
                      out  float4 OutColor0    : COLOR0,
                      out  float2 OutTexCoord0 : TEXCOORD0,
                      out  float4 OutCxForm0   : TEXCOORD1,
                      out  float4 OutCxForm1   : TEXCOORD2)                                            
{
   float4 InPosition       = fetchPosition(Index);
   float4 InTexCoord0      = fetchTexCoord0(Index);
   float4 InColor0         = fetchColor0(Index);   
   float4 InParameterIndex = fetchTexCoord1(Index);

   float4 InMatrixV0       = fetchTexCoord2(InParameterIndex.x);
   float4 InMatrixV1       = fetchTexCoord3(InParameterIndex.x);
   float4 InCxForm0        = fetchTexCoord4(InParameterIndex.x);
   float4 InCxForm1        = fetchTexCoord5(InParameterIndex.x);   
   
   InPosition.z = 0.0f;
   InPosition.w = 1.0f;
   OutPosition = InPosition;
   OutPosition.x = dot(InPosition, InMatrixV0);
   OutPosition.y = dot(InPosition, InMatrixV1);
   OutPosition.z = 0.0f;
   OutPosition.w = 1.0f;


   OutColor0    = InColor0.bgra;
   OutTexCoord0 = InTexCoord0;   
   OutCxForm0   = InCxForm0;
   OutCxForm1   = InCxForm1;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void flashUberGlyphPS(in  float2 InTexCoord0 : TEXCOORD0,
                      in  float4 InCxForm0   : TEXCOORD1,
                      in  float4 InCxForm1   : TEXCOORD2,
                      in  float4 InColor0    : COLOR0,
                      out float4 OutColor0   : COLOR0,
                      uniform bool bPremultipliedColor)
{   
   InColor0.a = InColor0.a * tex2D(gTex0Sampler, InTexCoord0).a;
   OutColor0 = InColor0 * InCxForm0 + InCxForm1;

   if (bPremultipliedColor)
   {
      OutColor0.rgb = OutColor0.rgb * OutColor0.a;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void flashVS( in int    Index       : INDEX,              
             out float4 OutPosition : POSITION,
             out float4 OutTexCoord0: TEXCOORD0,
             out float4 OutCxForm0  : TEXCOORD1,
             out float4 OutCxForm1  : TEXCOORD2,
             out float4 OutColor0   : COLOR0,
             out float4 OutColor1   : COLOR1 // Factor
             )
{
   float4 InPosition       = fetchPosition(Index);      
   float4 InColor0         = fetchColor0(Index);
   float4 InColor1         = fetchColor1(Index);
   float4 InParameterIndex = fetchTexCoord0(Index);
   
   OutCxForm0              = float4(1,1,1,1);
   OutCxForm1              = float4(0,0,0,0);

   float4 InMatrixV0 = fetchTexCoord3(InParameterIndex.x);
   float4 InMatrixV1 = fetchTexCoord4(InParameterIndex.x);
   float4 InTexGen0  = fetchTexCoord5(InParameterIndex.x);
   float4 InTexGen1  = fetchTexCoord6(InParameterIndex.x);
   float4 InTexGen2  = fetchTexCoord7(InParameterIndex.x);
   float4 InTexGen3  = fetchTexCoord8(InParameterIndex.x);
   float4 InCxForm0  = fetchTexCoord9(InParameterIndex.x);
   float4 InCxForm1  = fetchTexCoord10(InParameterIndex.x);
   float4 InColor2   = fetchTexCoord11(InParameterIndex.x);
   

   OutPosition   = InPosition;
   OutPosition.x = dot(InPosition, InMatrixV0);
   OutPosition.y = dot(InPosition, InMatrixV1);
   //OutPosition.z = 0.0f;
   //OutPosition.w = 1.0f;

   float4 InTex0 = float4(0,0,0,0);      
   InTex0.x = dot(InPosition, InTexGen0);
   InTex0.y = dot(InPosition, InTexGen1);
   InTex0.z = dot(InPosition, InTexGen2);
   InTex0.w = dot(InPosition, InTexGen3);

   OutColor0    = InColor0 * InColor2;
   OutColor1    = InColor1;
   OutTexCoord0 = InTex0; 
   OutCxForm0   = InCxForm0;
   OutCxForm1   = InCxForm1;   
   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void flashPS(   in  float4 InColor0  : COLOR0,
                in  float4 InColor1  : COLOR1,
                in  float4 InTex0    : TEXCOORD0,
                in  float4 InCxForm0 : TEXCOORD1,
                in  float4 InCxForm1 : TEXCOORD2,
                out float4 OutColor0 : COLOR0,
                
                //uniform bool bColor;
                //uniform bool bTex0,
                uniform bool bLerpTex0,
                uniform bool bLerpTex0Tex1,

                //uniform bool bComputeCxForm,
                //uniform bool bFactor,
                uniform bool bLerp1,
                uniform bool bPremultipliedColor// AC
                )
{
   OutColor0 = InColor0;

   if (bLerpTex0)
      OutColor0 = lerp(OutColor0, tex2D(gTex0Sampler, InTex0.xy), InColor1.b);

   if (bLerpTex0Tex1)
      OutColor0 = lerp(tex2D(gTex1Sampler, InTex0.zw), tex2D(gTex0Sampler, InTex0.xy), InColor1.b);

   OutColor0 = OutColor0 * InCxForm0 + InCxForm1;
   OutColor0.a *= InColor1.a;

   if (bLerp1)
      OutColor0 = lerp(1, OutColor0, OutColor0.a);

   if (bPremultipliedColor)
   {
      OutColor0.rgb = OutColor0.rgb * OutColor0.a;
   }

   //OutColor0 = float4(1,0,0,1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
technique flashTechnique
{
   pass SolidColor // identity factor - identity CXForm
   //pass CxFormGauraud // factor / cxform
   //pass CxFormGauraudNoAddAlpha // identity factor   
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(false, false, false, false);
   }

   pass CxFormTextureMultiply // identity factor
   //pass CxFormGauraudMultiply // factor / cxform
   //pass CxFormGauraudMultiplyNoAddAlpha //identity factor
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(false, false, true, false);
   }
   
   pass CxFormTexture // identity factor
   //pass CxFormGauraudTexture // 
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(true, false, false, false);
   }

   pass CxFormGauraudMultiplyTexture
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(true, false, true, false);
   }

   pass CxForm2Texture
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(false, true, false, false);
   }

   pass CxFormMultiply2Texture
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(false, true, true, false);
   }

   pass TextTextureColor
   {
      VertexShader = compile vs_3_0 flashGlyphVS();
      PixelShader  = compile ps_3_0 flashGlyphPS(false);
   }

   pass UberTextTextureColor
   {
      VertexShader = compile vs_3_0 flashUberGlyphVS();
      PixelShader  = compile ps_3_0 flashUberGlyphPS(false);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
technique flashTechniqueAC
{
   pass SolidColor // identity factor - identity CXForm
   //pass CxFormGauraud // factor / cxform
   //pass CxFormGauraudNoAddAlpha // identity factor   
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(false, false, false, true);
   }

   pass CxFormTextureMultiply // identity factor
   //pass CxFormGauraudMultiply // factor / cxform
   //pass CxFormGauraudMultiplyNoAddAlpha //identity factor
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(false, false, true, true);
   }
   
   pass CxFormTexture // identity factor
   //pass CxFormGauraudTexture // 
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(true, false, false, true);
   }

   pass CxFormGauraudMultiplyTexture
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(true, false, true, true);
   }

   pass CxForm2Texture
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(false, true, false, true);
   }

   pass CxFormMultiply2Texture
   {
      VertexShader = compile vs_3_0 flashVS();
      PixelShader  = compile ps_3_0 flashPS(false, true, true, true);
   }

   pass TextTextureColor
   {
      VertexShader = compile vs_3_0 flashGlyphVS();
      PixelShader  = compile ps_3_0 flashGlyphPS(true);
   }

   pass UberTextTextureColor
   {
      VertexShader = compile vs_3_0 flashUberGlyphVS();
      PixelShader  = compile ps_3_0 flashUberGlyphPS(true);
   }
}

