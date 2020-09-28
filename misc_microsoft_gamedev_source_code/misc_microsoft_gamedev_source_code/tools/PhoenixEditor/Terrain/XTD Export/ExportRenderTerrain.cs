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
using Terrain;
/*
 * CLM 04.16.07
 * This file renders the terrain in a faster manner for Exporting
 * 
 */

namespace Export360
{
   //--------------------------------------------
   public class ExportRenderTerrain
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
         TerrainGlobals.getVisual().getLODVB(ref mVB, ref mNumVerts, 0);
         mNumPrims = (uint)((mNumVerts) / 3);

         mPositionsTexture = TerrainGlobals.getVisual().giveEntireTerrainInTexture();

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
         mTerrainGPUShader = BRenderDevice.getShaderManager().getShader(CoreGlobals.getWorkPaths().mEditorShaderDirectory + "\\gpuTerrainExport.fx", mainShaderParams);
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
         mTerrainGPUShader.mShader.SetValue(mShaderNumWorldXVertsHandle, TerrainGlobals.getTerrain().getNumXVerts());
         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.None);

         mTerrainGPUShader.mShader.Begin(0);
      }
      public void postRender()
      {
         mTerrainGPUShader.mShader.End();
         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.CounterClockwise);
      }

      
      void renderLeafNode(BTerrainQuadNode node)
      {
         int lod = (int)BTerrainVisual.eLODLevel.cLOD0;
         BTerrainQuadNodeDesc desc = node.getDesc();
         float scale = TerrainGlobals.getTerrain().getTileScale() * ((int)Math.Pow(2, lod));
         Vector4 DataVals = new Vector4(2 * (BTerrainQuadNode.getMaxNodeWidth() >> lod), scale, desc.mMinXVert, desc.mMinZVert);
         mTerrainGPUShader.mShader.SetValue(mShaderDataValsHandle, DataVals);
         mTerrainGPUShader.mShader.CommitChanges();
         BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, (int)mNumPrims);
      }

      public void renderAll()
      {
         //set LOD Level
         mTerrainGPUShader.mShader.SetValue(mShaderChunkScaleHandle, TerrainGlobals.getTerrain().getTileScale());
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);

         mTerrainGPUShader.mShader.BeginPass((int)mPassNum);

         BTerrainQuadNode[] mNodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
         for (int i = 0; i < mNodes.Length; i++)
         {
            renderLeafNode(mNodes[i]);
         }

         mTerrainGPUShader.mShader.EndPass();
      }
      public void renderSubWindow(int minXChunk, int minZChunk, int maxXChunk, int maxZChunk)
      {
         int numXNodes = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
         minXChunk = (int)(BMathLib.Clamp(minXChunk,0,numXNodes-1));
         minZChunk = (int)(BMathLib.Clamp(minZChunk,0,numXNodes-1));
         maxXChunk = (int)(BMathLib.Clamp(maxXChunk,0,numXNodes-1));
         maxZChunk = (int)(BMathLib.Clamp(maxZChunk,0,numXNodes-1));

         //set LOD Level
         mTerrainGPUShader.mShader.SetValue(mShaderChunkScaleHandle, TerrainGlobals.getTerrain().getTileScale());
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);

         mTerrainGPUShader.mShader.BeginPass((int)mPassNum);

         BTerrainQuadNode[] mNodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();

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
}