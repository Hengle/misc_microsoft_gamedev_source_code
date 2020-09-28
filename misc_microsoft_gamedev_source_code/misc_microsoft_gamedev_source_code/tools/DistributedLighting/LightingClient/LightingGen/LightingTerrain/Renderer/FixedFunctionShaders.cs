using System;
using System.Drawing;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Text;
using System.IO;
using System.Diagnostics;

using EditorCore;

namespace Rendering
{

   public class FixedFuncShaders
   {
      
      public enum eFixedFuncShaderIndex
      {
         // All vertex shaders transform the incoming position by c0,c1,c2,c3 and minimally expect a position and color in the stream.
         // The matrix in c0-c3 must be transposed.
         cPosVS,
         cPosTex1VS,
         cPosDiffuseVS,
         cPosDiffuseTex1VS,
         cPosDiffuseTex2VS,
         cPosConstantDiffuseVS,

         cFirstPSIndex,

         // Iterated diffuse only
         cDiffusePS = cFirstPSIndex,

         // Iterated diffuse * tex0
         cDiffuseTex1PS,

         // Iterated diffuse * tex0 * tex1
         cDiffuseTex2PS,

         // White
         cWhitePS,

         // tex0
         cTex1PS,

         // tex0 * tex1
         cTex2PS,

         cRedVisPS,
         cGreenVisPS,
         cBlueVisPS,

         cNumFixedFuncShaders,

         cFixedFuncForceDWORD = 0x7FFFFFFF
      };
      public FixedFuncShaders()
      {

      }
      ~FixedFuncShaders()
      {

      }

      public void init()
      {
        
      }
      public void deinit()
      {
         for(int i=0;i<mVertexShaders.Count;i++)
         {
            if (mVertexShaders[i].mVS != null)
            {
               mVertexShaders[i].mVS.Dispose();
               mVertexShaders[i].mVS = null;
            }
         }
         mVertexShaders.Clear();

         for (int i = 0; i < mPixelShaders.Count; i++)
         {
            if (mPixelShaders[i].mPS != null)
            {
               mPixelShaders[i].mPS.Dispose();
               mPixelShaders[i].mPS = null;
            }
         }
         mPixelShaders.Clear();
      }

      List<BFixedFuncVS> mVertexShaders = new List<BFixedFuncVS>();
      List<BFixedFuncPS> mPixelShaders = new List<BFixedFuncPS>();

      public VertexShader getVertexShader(eFixedFuncShaderIndex shaderIndex)
      {
         for (int i = 0; i < mVertexShaders.Count; i++)
         {
            if (mVertexShaders[i].mID == shaderIndex)
               return mVertexShaders[i].mVS;
         }
         return null;
      }
      public PixelShader getPixelShader(eFixedFuncShaderIndex shaderIndex)
      {
         for (int i = 0; i < mPixelShaders.Count; i++)
         {
            if (mPixelShaders[i].mID == shaderIndex)
               return mPixelShaders[i].mPS;
         }
         return null;
      }




      string gpFixedFuncPS =
      "sampler gSampler0 : register(s0) = sampler_state  \n" +
      "{                                                 \n" +
      "   AddressU   = CLAMP;                            \n" +
      "   AddressV   = CLAMP;                            \n" +
      "   AddressW   = CLAMP;                            \n" +
      "   MipFilter  = LINEAR;                           \n" +
      "   MinFilter  = LINEAR;                           \n" +
      "   MagFilter  = LINEAR;                           \n" +
      "};                                                \n" +
      "sampler gSampler1 : register(s1);                 \n" +
      "struct InParams                                   \n" +
      "{                                                 \n" +
      "#ifdef DIFFUSE                                    \n" +
      "   float4 color0 : COLOR0;                        \n" +
      "#endif                                            \n" +
      "#ifdef SPEC                                       \n" +
      "   float4 color1 : COLOR1;                        \n" +
      "#endif                                            \n" +
      "#ifdef UV0                                        \n" +
      "   float2 tex0 : TEXCOORD0;                       \n" +
      "#endif                                            \n" +
      "#ifdef UV1                                        \n" +
      "   float2 tex1 : TEXCOORD1;                       \n" +
      "#endif                                            \n" +
      "};                                                \n" +
      "float4 main(InParams params) : COLOR              \n" +
      "{                                                 \n" +
      "#ifdef DIFFUSE                                    \n" +
      "   float4 c = params.color0;                      \n" +
      "#else                                             \n" +
      "   float4 c = float4(1,1,1,1);                    \n" +
      "#endif                                            \n" +
      "#ifdef UV0                                        \n" +
      "   c = c * tex2D(gSampler0, params.tex0);         \n" +
      "#endif                                            \n" +
      "#ifdef UV1                                        \n" +
      "   c = c * tex2D(gSampler1, params.tex1);         \n" +
      "#endif                                            \n" +
      "#ifdef SPEC                                       \n" +
      "   c = c + params.color1;                         \n" +
      "#endif                                            \n" +
      "#ifdef  OVERBRIGHT                                \n" +
      "   c *= 2.0f;                                     \n" +
      "#endif                                            \n" +
      "   return c;                                      \n" +
      "}                                                 \n";

      string gpDepthVisPS =
      "sampler gSampler0 : register(s0);                       \n" +
      "float4 gMul : register(c0);                             \n" +
      "float4 gAdd : register(c1);                             \n" +
      "float gSlice : register(c2);                            \n" +
      "float4 main(float2 tex0 : TEXCOORD0) : COLOR            \n" +
      "{                                                       \n" +
      "   float depth = tex3D(gSampler0, float3(tex0.x, tex0.y, gSlice)).r;\n" +
         //"   depth -= 8388608.0;                                  \n"
         //"   depth *= (1.0 / 8388608.0);                          \n"
      "   return gMul * depth + gAdd;                          \n" +
      "}                                                       \n";

      string gpAlphaVisPS =
      "sampler gSampler0 : register(s0);                       \n" +
      "float4 main(float2 tex0 : TEXCOORD0) : COLOR            \n" +
      "{                                                       \n" +
      "   return tex2D(gSampler0, tex0).a;                     \n" +
      "}                                                       \n";

      string gpRedVisPS =
      "sampler gSampler0 : register(s0);                       \n" +
      "float4 main(float2 tex0 : TEXCOORD0) : COLOR            \n" +
      "{                                                       \n" +
      "   return tex2D(gSampler0, tex0).r;                     \n" +
      "}                                                       \n";

      string gpGreenVisPS =
      "sampler gSampler0 : register(s0);                       \n" +
      "float4 main(float2 tex0 : TEXCOORD0) : COLOR            \n" +
      "{                                                       \n" +
      "   return tex2D(gSampler0, tex0).g;                     \n" +
      "}                                                       \n";

      string gpBlueVisPS =
      "sampler gSampler0 : register(s0);                       \n" +
      "float4 main(float2 tex0 : TEXCOORD0) : COLOR            \n" +
      "{                                                       \n" +
      "   return tex2D(gSampler0, tex0).b;                     \n" +
      "}                                                       \n";

      string gpFixedFuncVS =
      "float4x4 gWorldViewProjMatrix : register(c0);            \n" +
      "#ifdef USE_CONSTANT_DIFFUSE                              \n" +
      "float4   gColorConstant       : register(c4);            \n" +
      "#endif                                                   \n" +
      "void main(                                               \n" +
      "   in float4 InPosition : POSITION                       \n" +
      "#ifdef DIFFUSE                                           \n" +
      "   ,in  float4 InColor0 : COLOR0                         \n" +
      "#endif                                                   \n" +
      "#ifdef SPEC                                              \n" +
      "   ,in float4 InColor1 : COLOR1                          \n" +
      "#endif                                                   \n" +
      "#ifdef UV0                                               \n" +
      "   ,in float4 InTex0 : TEXCOORD0                         \n" +
      "#endif                                                   \n" +
      "#ifdef UV1                                               \n" +
      "   ,in float4 InTex1 : TEXCOORD1                         \n" +
      "#endif                                                   \n" +
      "   ,out float4 OutPosition : POSITION                    \n" +
      "#if defined(DIFFUSE) || defined(USE_CONSTANT_DIFFUSE)    \n" +
      "   ,out float4 OutColor0 : COLOR0                        \n" +
      "#endif                                                   \n" +
      "#ifdef SPEC                                              \n" +
      "   ,out float4 OutColor1 : COLOR1                        \n" +
      "#endif                                                   \n" +
      "#ifdef UV0                                               \n" +
      "   ,out float4 OutTex0 : TEXCOORD0                       \n" +
      "#endif                                                   \n" +
      "#ifdef UV1                                               \n" +
      "   ,out float4 OutTex1 : TEXCOORD1                       \n" +
      "#endif                                                   \n" +
      "   )                                                     \n" +
      "{                                                        \n" +
      "   OutPosition = mul(InPosition, gWorldViewProjMatrix);  \n" +
      "#ifdef DIFFUSE                                           \n" +
      "   OutColor0   = InColor0;                               \n" +
      "#endif                                                   \n" +
      "#ifdef USE_CONSTANT_DIFFUSE                              \n" +
      "   OutColor0   = gColorConstant;                         \n" +
      "#endif                                                   \n" +
      "#ifdef SPEC                                              \n" +
      "   OutColor1 = InColor1;                                 \n" +
      "#endif                                                   \n" +
      "#ifdef UV0                                               \n" +
      "   OutTex0 = InTex0;                                     \n" +
      "#endif                                                   \n" +
      "#ifdef UV1                                               \n" +
      "   OutTex1 = InTex1;                                     \n" +
      "#endif                                                   \n" +
      "}                                                        \n";

      class BFixedFuncVS
      {
         public FixedFuncShaders.eFixedFuncShaderIndex mID;
         public VertexShader mVS;
      };

      class BFixedFuncPS
      {
         public FixedFuncShaders.eFixedFuncShaderIndex mID;
         public PixelShader mPS;
      };

      private void compileShader(FixedFuncShaders.eFixedFuncShaderIndex id, string shaderText, string[] defines, bool vertexShader)
      {

         Macro []Macrodefines = null;
         if(defines!=null)
         {
            Macrodefines = new Macro[defines.Length];
            for (int k = 0; k < defines.Length; k++)
               Macrodefines[k].Definition = defines[k];
         }
         GraphicsStream gs = ShaderLoader.CompileShader(shaderText, "main", Macrodefines, null, vertexShader ? "vs_2_0" : "ps_2_0", ShaderFlags.None);

         if (vertexShader)
         {
            BFixedFuncVS vsA = new BFixedFuncVS();
            vsA.mVS = new VertexShader(BRenderDevice.getDevice(), gs);
            vsA.mID = id;
            mVertexShaders.Add(vsA);
         }
         else
         {
            BFixedFuncPS vsA = new BFixedFuncPS();
            vsA.mPS = new PixelShader(BRenderDevice.getDevice(), gs);
            vsA.mID = id;
            mPixelShaders.Add(vsA);
         }
         gs.Close();
         gs = null;
      }
      private void generateShaders()
      {
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cPosVS, gpFixedFuncVS, null, true);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cPosTex1VS, gpFixedFuncVS, new string[] { "UV0"}, true);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cPosDiffuseVS, gpFixedFuncVS, new string[] { "DIFFUSE" }, true);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cPosDiffuseTex1VS, gpFixedFuncVS, new string[] { "DIFFUSE", "UV0" }, true);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cPosDiffuseTex2VS, gpFixedFuncVS, new string[] { "DIFFUSE", "UV0", "UV0" }, true);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cPosConstantDiffuseVS, gpFixedFuncVS, new string[] { "USE_CONSTANT_DIFFUSE" }, true);

         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cDiffusePS, gpFixedFuncPS, new string[] { "DIFFUSE" }, false);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cDiffuseTex1PS, gpFixedFuncPS, new string[] { "DIFFUSE", "UV0" }, false);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cDiffuseTex2PS, gpFixedFuncPS, new string[] { "DIFFUSE", "UV0", "UV1" }, false);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cWhitePS, gpFixedFuncPS, null, false);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cTex1PS, gpFixedFuncPS, new string[] { "UV0" }, false);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cTex2PS, gpFixedFuncPS, new string[] { "UV0", "UV1" }, false);

         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cRedVisPS, gpRedVisPS, null, false);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cGreenVisPS, gpGreenVisPS, null, false);
         compileShader(FixedFuncShaders.eFixedFuncShaderIndex.cBlueVisPS, gpBlueVisPS, null, false);

      }

   }

 

}