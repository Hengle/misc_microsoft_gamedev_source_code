using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.ComponentModel;
using EditorCore;
using Rendering;


/*
 * CLM 04.16.07
 * This file renders the terrain in a faster manner for Exporting
 * 
 */

namespace LightingClient
{
   //--------------------------------------------
   public partial class ExportRenderTerrain
   {
      Texture mPositionsTexture=null;
      VertexBuffer mVB = null;
      uint mNumVerts = 0;
      uint mNumPrims = 0;
      int mPassNum = 0;
      public ShaderHandle mTerrainGPUShader = null;
      EffectHandle mShaderPositionsTexHandle = null;
      EffectHandle mShaderWVPHandle = null;
      EffectHandle mShaderDataValsHandle = null;
      EffectHandle mShaderChunkScaleHandle = null;
      EffectHandle mShaderNumWorldXVertsHandle = null;

      public void init()
      {
         TerrainGlobals.getLODVB().getLODVB(ref mVB, ref mNumVerts, 0);
         mNumPrims = (uint)((mNumVerts) / 3);

         mPositionsTexture = TerrainGlobals.getTerrain().giveEntireTerrainInTexture();

         loadShader();
      }
      public void destroy()
      {
         if(mPositionsTexture!=null)
         {
            mPositionsTexture.Dispose();
            mPositionsTexture = null;
         }
         
         mVB = null;

         if (mTerrainGPUShader!=null)
         {
            BRenderDevice.getShaderManager().freeShader(mTerrainGPUShader.mFilename);
            mTerrainGPUShader.mShader.Dispose();
            mTerrainGPUShader.mShader = null;
            mTerrainGPUShader = null;
         }
         
      }

      public void mainShaderParams(string filename)
      {
         mShaderDataValsHandle = mTerrainGPUShader.getEffectParam("g_terrainVals");
         mShaderPositionsTexHandle = mTerrainGPUShader.getEffectParam("positionsTexture");
         mShaderWVPHandle = mTerrainGPUShader.getEffectParam("worldViewProj");
         mShaderChunkScaleHandle = mTerrainGPUShader.getEffectParam("g_chunkScale");
         mShaderNumWorldXVertsHandle = mTerrainGPUShader.getEffectParam("g_numWorldXVerts");
      }

      void loadShader()
      {
        // mTerrainGPUShader = BRenderDevice.getShaderManager().getShader(AppDomain.CurrentDomain.BaseDirectory + "\\shaders\\gpuTerrainExport.fx", mainShaderParams);
         mTerrainGPUShader =  new ShaderHandle();
         mTerrainGPUShader.loadFromString(mShaderString, mainShaderParams);

         mainShaderParams(null);
      }

      public void setPassNum(int passNum)
      {
         mPassNum = passNum;
      }

      public void preRenderSetup()
      {
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;

         Matrix g_matView;
         Matrix g_matProj;
         g_matView = BRenderDevice.getDevice().Transform.View;
         g_matProj = BRenderDevice.getDevice().Transform.Projection;
         Matrix worldViewProjection = g_matView * g_matProj;

         mTerrainGPUShader.mShader.SetValue(mShaderWVPHandle, worldViewProjection);
         mTerrainGPUShader.mShader.SetValue(mShaderPositionsTexHandle, mPositionsTexture);
         mTerrainGPUShader.mShader.SetValue(mShaderNumWorldXVertsHandle, TerrainGlobals.getTerrain().mNumXVerts);
         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.None);

         mTerrainGPUShader.mShader.Begin(0);
      }
      public void postRender()
      {
         mTerrainGPUShader.mShader.End();
         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.CounterClockwise);
      }


      void renderLeafNode(BTerrainQuadNodeDesc desc)
      {
         int lod = (int)BTerrainVisualLODData.eLODLevel.cLOD0;
         float scale = TerrainGlobals.getTerrain().mTileScale * ((int)Math.Pow(2, lod));
         Vector4 DataVals = new Vector4(2 * (BTerrainQuadNodeDesc.cMaxWidth >> lod), scale, desc.mMinXVert, desc.mMinZVert);
         mTerrainGPUShader.mShader.SetValue(mShaderDataValsHandle, DataVals);
         mTerrainGPUShader.mShader.CommitChanges();
         BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, (int)mNumPrims);
      }

      public void renderAll()
      {
         //set LOD Level
         mTerrainGPUShader.mShader.SetValue(mShaderChunkScaleHandle, TerrainGlobals.getTerrain().mTileScale);
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);

         mTerrainGPUShader.mShader.BeginPass((int)mPassNum);
            
         BTerrainQuadNodeDesc[] mNodes = TerrainGlobals.getTerrain().mQuadNodeDescArray;
         for (int i = 0; i < mNodes.Length; i++)
         {
            renderLeafNode(mNodes[i]);
         }

         mTerrainGPUShader.mShader.EndPass();
      }
      public void renderSubWindow(int minXChunk, int minZChunk, int maxXChunk, int maxZChunk)
      {
         int numXNodes = (int)(TerrainGlobals.getTerrain().mNumXVerts / BTerrainQuadNodeDesc.cMaxWidth);
         minXChunk = (int)(BMathLib.Clamp(minXChunk,0,numXNodes-1));
         minZChunk = (int)(BMathLib.Clamp(minZChunk,0,numXNodes-1));
         maxXChunk = (int)(BMathLib.Clamp(maxXChunk,0,numXNodes-1));
         maxZChunk = (int)(BMathLib.Clamp(maxZChunk,0,numXNodes-1));

         //set LOD Level
         mTerrainGPUShader.mShader.SetValue(mShaderChunkScaleHandle, TerrainGlobals.getTerrain().mTileScale);
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);

         mTerrainGPUShader.mShader.BeginPass((int)mPassNum);

         BTerrainQuadNodeDesc[] mNodes = TerrainGlobals.getTerrain().mQuadNodeDescArray;

         for (int i = minXChunk; i < maxXChunk; i++)
         {
            for (int j = minZChunk; j < maxZChunk; j++)
            {
               int indx = i * numXNodes + j;
               renderLeafNode(mNodes[indx]);
            }
               
         }

         mTerrainGPUShader.mShader.EndPass();
      }
   }

   partial class ExportRenderTerrain
   {
      const string mShaderString =
         "float4x4 world			: World;	\n" +
      "float4x4 worldViewProj	: WorldViewProjection;\n" +
      "texture positionsTexture; \n" +
      "sampler vertSampler = sampler_state\n" +
      "{          \n" +
      "    Texture   = (positionsTexture);\n" +
      "    MipFilter = NONE;\n" +
      "    MinFilter = POINT;\n" +
      "    MagFilter = POINT;\n" +
      "    AddressU = CLAMP;\n" +
      "    AddressV = CLAMP;\n" +
      "};            \n" +

      "uniform float4 g_terrainVals;         \n" +//numXTiles, scale, minXVert,minZVert
      "uniform float  g_chunkScale = 0.25;\n" +
      "uniform float g_numWorldXVerts = 256;\n" +

      "struct VS_INPUT{    float3 position	: POSITION;};\n" +
      "struct VS_OUTPUT{    float4 hposition	: POSITION;    float4 rPos			: TEXCOORD5;};\n" +

      "void GiveVertFromSampler(float3 wPos, float2 uvCoord, out float3 nPos)\n" +
      "{          \n" +
      "   nPos = float3(0,0,0);\n" +
      "   float4 position = tex2Dlod( vertSampler,   float4(uvCoord.y,uvCoord.x,0.f,0.f) );\n" +
      "   nPos = position.xyz;\n" +
      "   wPos.xz = (wPos.xz*g_terrainVals.y)+(g_terrainVals.zw*g_chunkScale) ; \n" +
      "   nPos+=wPos;\n" +
      "}       \n" +

      "float3 VIndexToPos(int vIndex)\n" +
      "{       \n" +
      "   float sizeDivX    = 1.f / float(g_terrainVals.x);\n" +
      "   float3 nPos       =  float3(0,0,0);\n" +
      "   nPos.x =  floor(vIndex * sizeDivX);\n" +
      "   nPos.z =  ceil(fmod(vIndex, g_terrainVals.x)-0.003);\n" +
      "   float2 uvCoord0 = (float2(nPos.x, nPos.z) + g_terrainVals.zw)/ g_numWorldXVerts;//float(g_terrainVals.x);\n" +
      "   GiveVertFromSampler(nPos, uvCoord0, nPos);\n" +
      "   return nPos;\n" +
      "}\n" +


      "VS_OUTPUT myvs( VS_INPUT IN )\n" +
      "{\n" +
      "    VS_OUTPUT OUT;\n" +
      "	float3 pos = VIndexToPos(int(IN.position.x));\n" +
      "	float4 tPos = mul( float4(pos.x,pos.y,pos.z,1.f ),worldViewProj);\n" +
      "	OUT.hposition = tPos;\n" +
      "	OUT.rPos = tPos;\n" +
      "	return OUT;\n" +
      "}\n" +

      "texture prevDepthTexture;\n" +
      "sampler prevDepthSampler = sampler_state\n" +
      "{\n" +
      "    Texture   = (prevDepthTexture);\n" +
      "    MipFilter = Point;\n" +
      "    MinFilter = Point;\n" +
      "    MagFilter = Point;\n" +
      "    \n" +
      "    AddressU = WRAP;\n" +
      "    AddressV = WRAP;\n" +
      "    AddressW = CLAMP;\n" +
      "};\n" +

      "float gWindowWidth = 256;\n" +

      "float4 depthPeel( VS_OUTPUT IN ) : COLOR\n" +
      "{\n" +
      "	float4 rasterPt = IN.rPos / IN.rPos.w;\n" +
      "	rasterPt.xy = 0.5f * rasterPt.xy  + float2(0.5f,0.5f);\n" +
      "	rasterPt.y = 1-rasterPt.y;		//Ugg??????\n" +
      "    float myDepth =  IN.rPos.z / IN.rPos.w;\n" +
      "    float prevDepth = tex2D(prevDepthSampler, rasterPt.xy);\n" +


          //clip me if i'm farther than previous depth
               //CLM this was 0.07 first, moving it to 0.01 made the entire thing darker 
               //althoug this may add extra passes to the depth peeling, causing more overhead (it was moved to 0.07 beacuse there were depth fragments that "SHOULD" not cause an extra pass, causing an extra pass..)
      "    float zBufPrecision = 0.01f; \n" +
      "    clip(prevDepth - (myDepth+zBufPrecision));\n" +


       "   return myDepth;\n" +
      "}\n" +

      "technique Technique0\n" +
      "{\n" +
      "    pass DepthPeel\n" +
      "    {\n" +
      "		VertexShader = compile vs_3_0 myvs();\n" +
      "		PixelShader  = compile ps_3_0 depthPeel();\n" +
       "   } \n" +
      "}\n";
   };
}