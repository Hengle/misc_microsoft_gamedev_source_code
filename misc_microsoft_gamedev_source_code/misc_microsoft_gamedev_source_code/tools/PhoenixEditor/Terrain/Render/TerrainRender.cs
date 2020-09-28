using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using System.Collections.Generic;
using System;
using Rendering;
using EditorCore;
using SimEditor;

//-----------------------------------------------
namespace Terrain
{
   #region TerrainRender

   public class BTerrainRender
   {
      public BTerrainRender()
      {

      }

      #region SHADER PARAM GATHER
      public void mainShaderParams(string filename)
      {
         mShaderDataValsHandle = mTerrainGPUShader.getEffectParam("g_terrainVals");
         mShaderDataValsHandle2 = mTerrainGPUShader.getEffectParam("g_terrainVals2");

         mShaderPositionsTexHandle = mTerrainGPUShader.getEffectParam("positionsTexture");

         mShaderNormalsTexHandle = mTerrainGPUShader.getEffectParam("normalsTexture");
         mShaderAlphasTexHandle = mTerrainGPUShader.getEffectParam("vertalphasTexture");
         mShaderLightsTexHandle = mTerrainGPUShader.getEffectParam("vertLightTexture");
         mShaderTessellationTexHandle = mTerrainGPUShader.getEffectParam("vertTessTexture");

         mShaderSkirtTexHandle = mTerrainGPUShader.getEffectParam("skirtHeightsTexture");

         mShaderWVPHandle = mTerrainGPUShader.getEffectParam("worldViewProj");
         mShaderWHandle = mTerrainGPUShader.getEffectParam("world");

         mShaderSkirtNodeEnabledHandle = mTerrainGPUShader.getEffectParam("g_skirtNodeEnabled");
         mShaderWorldSizeHandle = mTerrainGPUShader.getEffectParam("g_worldSize");
         mShaderSkirtValsHandle = mTerrainGPUShader.getEffectParam("g_skirtVals");

         mShaderGridToggleHandle = mTerrainGPUShader.getEffectParam("gGridToggle");
         mShaderGridSpacingHandle = mTerrainGPUShader.getEffectParam("gGridSpacing");

         mShaderGridColor0Handle = mTerrainGPUShader.getEffectParam("gGridColor0");
         mShaderGridColor1Handle = mTerrainGPUShader.getEffectParam("gGridColor1");
         mShaderGridColor2Handle = mTerrainGPUShader.getEffectParam("gGridColor2");
         mShaderGridColor3Handle = mTerrainGPUShader.getEffectParam("gGridColor3");

         //TEXTURING
         mShaderAlbedoHandle = mTerrainGPUShader.getEffectParam("albedoTexture");
         mShaderNormalHandle = mTerrainGPUShader.getEffectParam("normalTexture");

         mShaderBlendsIndxHandle = mTerrainGPUShader.getEffectParam("g_blends");
         mShaderNumTexHandle = mTerrainGPUShader.getEffectParam("g_numTextures");

         mShaderNumBlendsHandle = mTerrainGPUShader.getEffectParam("g_numBlends");
         mShaderRCPNumBlendsHandle = mTerrainGPUShader.getEffectParam("g_rcpNumBlends");

         //   mShaderAlbedoHandle       = mTerrainGPUShader.GetParameter(null, "albedoTextureArray");
         //  mShaderNormalHandle       = mTerrainGPUShader.GetParameter(null, "normalTextureArray");

         mShaderAlphaTexHandle = mTerrainGPUShader.getEffectParam("alphasTexture");
         mShaderIndexTexHandle = mTerrainGPUShader.getEffectParam("indexesTexture");
         mShaderChunkScaleHandle = mTerrainGPUShader.getEffectParam("g_chunkScale");
         mShaderDirLightHandle = mTerrainGPUShader.getEffectParam("gDirLightDir");
         mShaderAmbientLightHandle = mTerrainGPUShader.getEffectParam("gAmbientLight");


         mShaderTexUVOffset = mTerrainGPUShader.getEffectParam("g_atlasUVOffset");
         mShaderAtlasScale = mTerrainGPUShader.getEffectParam("g_AtlasScale");

         mWorldCameraPosHandle = mTerrainGPUShader.getEffectParam("gWorldCameraPos");

         mPlanarFogEnabledHandle = mTerrainGPUShader.getEffectParam("gPlanarFogEnabled");
         mRadialFogColorHandle = mTerrainGPUShader.getEffectParam("gRadialFogColor");
         mPlanarFogColorHandle = mTerrainGPUShader.getEffectParam("gPlanarFogColor");
         mFogParamsHandle = mTerrainGPUShader.getEffectParam("gFogParams");

         mSkirtWireColorHandle = mTerrainGPUShader.getEffectParam("g_SkirtWireColor");

         mPerfColor = mTerrainGPUShader.getEffectParam("g_perfColor");
      }
      public void cursorShaderParams(string filename)
      {
         mCursorShaderDataValsHandle = mTerrainGPUCursorShader.getEffectParam("g_terrainVals");
         mCursorShaderDataValsHandle2 = mTerrainGPUCursorShader.getEffectParam("g_terrainVals2");
         mCursorShaderPositionsTexHandle = mTerrainGPUCursorShader.getEffectParam("positionsTexture");
         mCursorShaderNormalsTexHandle = mTerrainGPUCursorShader.getEffectParam("normalsTexture");
         mCursorShaderWVPHandle = mTerrainGPUCursorShader.getEffectParam("worldViewProj");

         mCursorShaderBrushHandle = mTerrainGPUCursorShader.getEffectParam("g_brushInfo");
         mCursorShaderInterPointHandle = mTerrainGPUCursorShader.getEffectParam("g_centerPoint");
         mCursorMaskHandle = mTerrainGPUCursorShader.getEffectParam("alphaMaskCursor");
         mCursorTexArrayHandle = mTerrainGPUCursorShader.getEffectParam("albedoTextureArray");
         mCursorTexDecalHandle = mTerrainGPUCursorShader.getEffectParam("albedoDecalTexture");
         mCursorColorHandle = mTerrainGPUCursorShader.getEffectParam("g_colorValue");
         mCursorChunkScaleHandle = mTerrainGPUCursorShader.getEffectParam("g_chunkScale");
      }

      public void widgetShaderParams(string filename)
      {
         mWidgetShaderDataValsHandle = mTerrainGPUWidgetShader.getEffectParam("g_terrainVals");
         mWidgetShaderDataValsHandle2 = mTerrainGPUWidgetShader.getEffectParam("g_terrainVals2");
         mWidgetShaderPositionsTexHandle = mTerrainGPUWidgetShader.getEffectParam("positionsTexture");
         mWidgetShaderNormalsTexHandle = mTerrainGPUWidgetShader.getEffectParam("normalsTexture");
         mWidgetShaderWVPHandle = mTerrainGPUWidgetShader.getEffectParam("worldViewProj");

         
         mWidgetChunkScaleHandle = mTerrainGPUWidgetShader.getEffectParam("g_chunkScale");
      }
      #endregion
      public bool init()
      {

         mTerrainGPUShader = BRenderDevice.getShaderManager().getShader(CoreGlobals.getWorkPaths().mEditorShaderDirectory + "\\gpuTerrainEditor.fx", mainShaderParams);
         mainShaderParams(null);
         
         
         //CURSOR
         mTerrainGPUCursorShader = BRenderDevice.getShaderManager().getShader(CoreGlobals.getWorkPaths().mEditorShaderDirectory + "\\gpuRenderCursor.fx", cursorShaderParams);
         cursorShaderParams(null);

         //WIDGETS
         mTerrainGPUWidgetShader = BRenderDevice.getShaderManager().getShader(CoreGlobals.getWorkPaths().mEditorShaderDirectory + "\\gpuTerrainWidgets.fx", widgetShaderParams);
         widgetShaderParams(null);
         


         // Create skirt textures
         int minSize = TerrainGlobals.getTerrain().getNumSkirtXVertsPerQuadrant();
         int sizePowerOfTwo = 2;
         while (sizePowerOfTwo < minSize) { sizePowerOfTwo = sizePowerOfTwo << 1; }
         mSkirtTextureSizeX = sizePowerOfTwo;

         minSize = TerrainGlobals.getTerrain().getNumSkirtZVertsPerQuadrant();
         sizePowerOfTwo = 2;
         while (sizePowerOfTwo < minSize) { sizePowerOfTwo = sizePowerOfTwo << 1; }
         mSkirtTextureSizeY = sizePowerOfTwo;

         for (int i = 0; i < 8; i++)
         {
            mSkirtTexture[i] = new Texture(BRenderDevice.getDevice(), mSkirtTextureSizeX, mSkirtTextureSizeY, 1, Usage.Dynamic, Format.A32B32G32R32F, Pool.Default);
         }

         updateAllSkirtTextures();

         // Create skirt wireframe overlay vertex buffer
         makeSkirtOffsetsVB();


         return true;
      }
      public void destroy()
      {
         
         if (mTerrainGPUShader != null)
         {
            BRenderDevice.getShaderManager().freeShader(mTerrainGPUShader.mFilename);
            mTerrainGPUShader.destroy();
            mTerrainGPUShader = null;
         }
         if (mTerrainGPUWidgetShader != null)
         {
            BRenderDevice.getShaderManager().freeShader(mTerrainGPUWidgetShader.mFilename);
            mTerrainGPUWidgetShader.destroy();
            mTerrainGPUWidgetShader = null;
         }
         if (mTerrainGPUCursorShader != null)
         {
            BRenderDevice.getShaderManager().freeShader(mTerrainGPUCursorShader.mFilename);
            mTerrainGPUCursorShader.destroy();
            mTerrainGPUCursorShader = null;
         }
         

         // Release all skirt textures
         for (int i = 0; i < 8; i++)
         {
            if (mSkirtTexture[i] != null)
            {
               mSkirtTexture[i].Dispose();
               mSkirtTexture[i] = null;
            }
         }

         // Release skirt VB
         if (mSkirtOffsetVertexBuffer != null)
         {
            mSkirtOffsetVertexBuffer.Dispose();
            mSkirtOffsetVertexBuffer = null;
         }
      }

      public void deviceReset()
      {
         // Release all skirt textures
         for (int i = 0; i < 8; i++)
         {
            if (mSkirtTexture[i] != null)
            {
               mSkirtTexture[i].Dispose();
               mSkirtTexture[i] = null;
            }
         }

         // Release skirt VB
         if (mSkirtOffsetVertexBuffer != null)
         {
            mSkirtOffsetVertexBuffer.Dispose();
            mSkirtOffsetVertexBuffer = null;
         }



         // Recreate Textures
         for (int i = 0; i < 8; i++)
         {
            mSkirtTexture[i] = new Texture(BRenderDevice.getDevice(), mSkirtTextureSizeX, mSkirtTextureSizeY, 1, Usage.Dynamic, Format.A32B32G32R32F, Pool.Default);
         }

         updateAllSkirtTextures();

         // Create skirt wireframe overlay vertex buffer
         makeSkirtOffsetsVB();
      }



      public void preRenderSetup()
      {
         //VP Matrix
         Matrix g_matView;
         Matrix g_matProj;
         g_matView = BRenderDevice.getDevice().Transform.View;
         g_matProj = BRenderDevice.getDevice().Transform.Projection;
         Matrix worldViewProjection = g_matView * g_matProj;

         mTerrainGPUShader.mShader.SetValue(mShaderWVPHandle, worldViewProjection);
         mTerrainGPUShader.mShader.SetValue(mShaderNumTexHandle, SimTerrainType.mActiveWorkingSet.Count);
         BRenderDevice.getDevice().VertexFormat = VertexFormats.Position;

         Vector3 pos = TerrainGlobals.getTerrainFrontEnd().mCameraManager.mEye;
         mTerrainGPUShader.mShader.SetValue(mWorldCameraPosHandle, new Vector4(pos.X, pos.Y, pos.Z, 1.0f));


         if (TerrainGlobals.getEditor().isRenderFogEnabled() && SimGlobals.getSimMain().GlobalLightSettings!=null)
         {
            System.Drawing.Color radialFogColor = SimGlobals.getSimMain().GlobalLightSettings.zFogColor;
            float radialFogDensity = SimGlobals.getSimMain().GlobalLightSettings.zFogDensity / 1000.0f;
            float radialFogStart = SimGlobals.getSimMain().GlobalLightSettings.zFogStart;
            float radialFogIntensity = SimGlobals.getSimMain().GlobalLightSettings.zFogIntensity;
            Vector4 radialFogColorVec = new Vector4((radialFogColor.R / 255.0f) * radialFogIntensity,
                                                   (radialFogColor.G / 255.0f) * radialFogIntensity,
                                                   (radialFogColor.B / 255.0f) * radialFogIntensity,
                                                   1.0f);

            System.Drawing.Color planarFogColor = SimGlobals.getSimMain().GlobalLightSettings.planarFogColor;
            float planarFogDensity = SimGlobals.getSimMain().GlobalLightSettings.planarFogDensity / 1000.0f;
            float planarFogStart = SimGlobals.getSimMain().GlobalLightSettings.planarFogStart;
            float planarFogIntensity = SimGlobals.getSimMain().GlobalLightSettings.planarFogIntensity;
            Vector4 planarFogColorVec = new Vector4((planarFogColor.R / 255.0f) * planarFogIntensity,
                                                   (planarFogColor.G / 255.0f) * planarFogIntensity,
                                                   (planarFogColor.B / 255.0f) * planarFogIntensity,
                                                   1.0f);

            bool planarFogEnable = (planarFogDensity > 0.0f) ? true : false;

            // Apply fog params
            mTerrainGPUShader.mShader.SetValue(mPlanarFogEnabledHandle, planarFogEnable);
            mTerrainGPUShader.mShader.SetValue(mRadialFogColorHandle, radialFogColorVec);
            mTerrainGPUShader.mShader.SetValue(mPlanarFogColorHandle, planarFogColorVec);
            mTerrainGPUShader.mShader.SetValue(mFogParamsHandle, new Vector4(radialFogDensity * radialFogDensity,
                                                                  radialFogStart * radialFogStart,
                                                                  planarFogDensity * planarFogDensity,
                                                                  planarFogStart));
         }
         else
         {
            // Apply fog params
            mTerrainGPUShader.mShader.SetValue(mPlanarFogEnabledHandle, false);
            mTerrainGPUShader.mShader.SetValue(mFogParamsHandle, new Vector4(0.0f,
                                                                     float.MaxValue,
                                                                     0.0f,
                                                                     float.MinValue));
         }

         if (TerrainGlobals.getEditor().getRenderMode() == BTerrainEditor.eEditorRenderMode.cRenderFullWireframe ||
            TerrainGlobals.getEditor().getRenderMode() == BTerrainEditor.eEditorRenderMode.cRenderFlatWireframe)
            BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);
         else if (TerrainGlobals.getEditor().getRenderMode() == BTerrainEditor.eEditorRenderMode.cRenderFull ||
            TerrainGlobals.getEditor().getRenderMode() == BTerrainEditor.eEditorRenderMode.cRenderFlat ||
            TerrainGlobals.getEditor().getRenderMode() == BTerrainEditor.eEditorRenderMode.cRenderFullOverlay)
            BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);
      }
      public void postRender()
      {
            BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);
      }

      private Vector4 mLightDir = new Vector4(1, 1, 1, 0);
      private float mAmbientLight = 0.5f;
      public void recalcMainDirLight(float sunRotation, float sunInclination)
      {
         //  Matrix mat = Matrix.RotationX(SimEditor.lightSetXML) * Matrix.RotationY(sunRotation);
         //   Vector4 unitY = new Vector4(0,1,0,0);
         //   unitY = Vector4.Transform(unitY,mat);

         //  mLightDir = -unitY;
      }

      public void renderEntireList(List<BTerrainQuadNode> mNodes, List<BTerrainQuadNodeRenderInstance> nodeInstances)
      {
         preRenderSetup();   //in SM3 this sets up all the shader vars

         List<int> mNeedsRendering = new List<int>();
         for (int i = 0; i < mNodes.Count; i++)
         {
            if (mNodes[i].getTextureData(nodeInstances[i].texturingLOD) != null && mNodes[i].getTextureData(nodeInstances[i].texturingLOD).mCompositeTexture != null)
            {
               render(mNodes[i],TerrainGlobals.getVisual().getVisualHandle( mNodes[i].mVisualDataIndxPerLOD[nodeInstances[i].geometryLOD]), mNodes[i].getTextureData(nodeInstances[i].texturingLOD), nodeInstances[i].matrix, nodeInstances[i].quadrant);
            }
            else
            {
               mNeedsRendering.Add(i);
            }
         }

         postRender();
         //WE DIDN'T HAVE ROOM IN THE CACHE FOR US

         for (int i = 0; i < mNeedsRendering.Count; i++)
         {
            int q = 0;
            TerrainGlobals.getTexturing().preConvertSetup();
            for (q = i; q < mNeedsRendering.Count; q++)
            {
               int indx = mNeedsRendering[q];
               if (mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD) == null || mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD).mCompositeTexture == null)
               {
                  //grab a node from the used list
                  BTerrainQuadNodeDesc mDesc = mNodes[indx].getDesc();
                  Vector3 center = (mDesc.m_min + mDesc.m_max) * 0.5f;

                  mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD).mCompositeTexture = TerrainGlobals.getTexturing().getCompositeTexture((int)(BTerrainTexturing.getTextureWidth() >> mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD).mTextureLODLevel), (int)(BTerrainTexturing.getTextureHeight() >> mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD).mTextureLODLevel));
                  if (mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD).mCompositeTexture == null)
                     break;

                  mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD).mCompositeTexture.mState = eCompositeTextureState.cCS_UsedThisFrame;
                  mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD).mCompositeTexture.mOwnerNode = mNodes[indx];

                  
                  TerrainGlobals.getTexturing().convertLayersToTexturingDataHandle( mNodes[indx].mLayerContainer,
                                                                                    mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD).mCompositeTexture,
                                                                                    mNodes[indx],
                                                                                    nodeInstances[indx].texturingLOD);
               }
            }
            TerrainGlobals.getTexturing().postConvertSetup();

            preRenderSetup();
            for (int k = i; k < q; k++)
            {
               int indx = mNeedsRendering[k];

               render(mNodes[i],TerrainGlobals.getVisual().getVisualHandle(mNodes[indx].mVisualDataIndxPerLOD[nodeInstances[indx].geometryLOD]), mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD), nodeInstances[indx].matrix, nodeInstances[indx].quadrant);
               mNodes[indx].getTextureData(nodeInstances[indx].texturingLOD).mCompositeTexture.mState = eCompositeTextureState.cCS_Used;
            }
            postRender();

            i = q - 1;
         }
      }

      public void render(BTerrainQuadNode node, BTerrainVisualDataHandle handle, BTerrainTexturingDataHandle texturing, Matrix world, int quadrant)
      {
         if (handle.mHandle == null)
            return;
         try
         {
            //VP Matrix
            Matrix g_matView;
            Matrix g_matProj;
            g_matView = BRenderDevice.getDevice().Transform.View;
            g_matProj = BRenderDevice.getDevice().Transform.Projection;
            Matrix worldViewProjection = world * g_matView * g_matProj;
            mTerrainGPUShader.mShader.SetValue(mShaderWHandle, world);
            mTerrainGPUShader.mShader.SetValue(mShaderWVPHandle, worldViewProjection);


            //SET UP VARIABLES
            //texturing
            if (mPassNum == (int)eRenderPass.eRP_Full || mPassNum == (int)eRenderPass.eRP_Select_Textured)
            {
               if (texturing.mCompositeTexture.UsingAtlas == false)
               {
                  mTerrainGPUShader.mShader.SetValue(mShaderAlbedoHandle, texturing.mCompositeTexture.mTextures[(int)BTerrainTexturing.eTextureChannels.cAlbedo]);
                  mTerrainGPUShader.mShader.SetValue(mShaderNormalHandle, texturing.mCompositeTexture.mTextures[(int)BTerrainTexturing.eTextureChannels.cNormal]);
               }
               else
               {
                  BTerrainCompositeAtlasTexture atlas = texturing.mCompositeTexture.mAtlas;
                  mTerrainGPUShader.mShader.SetValue(mShaderAlbedoHandle, atlas.mTextures[(int)BTerrainTexturing.eTextureChannels.cAlbedo]);
                  mTerrainGPUShader.mShader.SetValue(mShaderNormalHandle, atlas.mTextures[(int)BTerrainTexturing.eTextureChannels.cNormal]);

                  mTerrainGPUShader.mShader.SetValue(mShaderAtlasScale, atlas.mAtlasScale);
                  float[] uv = new float[2];
                  uv[0] = texturing.mCompositeTexture.mAtlasUVOffset.X;
                  uv[1] = texturing.mCompositeTexture.mAtlasUVOffset.Y;
                  mTerrainGPUShader.mShader.SetValue(mShaderTexUVOffset, uv);// new float[2] { 0, 0 });
               }
            }
            else if (mPassNum == (int)eRenderPass.eRP_ChunkPerf)
            {

               int numLayers = node.mLayerContainer.getNumNonBlankLayers();
               int num360PackedLayers = ((int)(numLayers / 4)) + 1;
               Vector4 col = new Vector4();
               if (num360PackedLayers == 1)
                  col=new Vector4(0, 1, 0, 1);
               else if (num360PackedLayers == 2)
                  col= new Vector4(1, 1, 0, 1);
               else
                 col= new Vector4(1, 0, 0, 1);

               mTerrainGPUShader.mShader.SetValue(mPerfColor, col);
               
            }
            else
            {
           //    mTerrainGPUShader.mShader.SetValue(mShaderAlbedoHandle, null);
            //   mTerrainGPUShader.mShader.SetValue(mShaderNormalHandle, null);

            }


            //Our position texture
            mTerrainGPUShader.mShader.SetValue(mShaderPositionsTexHandle, handle.mHandle.mPositionsTexture);
            mTerrainGPUShader.mShader.SetValue(mShaderNormalsTexHandle, handle.mHandle.mNormalsTexture!=null ?handle.mHandle.mNormalsTexture :
                                                                        BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_Green));
            mTerrainGPUShader.mShader.SetValue(mShaderAlphasTexHandle, handle.mHandle.mAlphasTexture != null ? handle.mHandle.mAlphasTexture :
                                                                        BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_White));
            mTerrainGPUShader.mShader.SetValue(mShaderLightsTexHandle, TerrainGlobals.getEditor().doShowLighting() ? handle.mHandle.mLightTexture : 
                                                                                       BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_Black));
            mTerrainGPUShader.mShader.SetValue(mShaderTessellationTexHandle, handle.mHandle.mTessellationTexture!=null?handle.mHandle.mTessellationTexture :
                                                                                       BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_Black));

            // Skirt texture
            Vector4 WorldSize = new Vector4(TerrainGlobals.getTerrain().getWorldSizeX(), TerrainGlobals.getTerrain().getWorldSizeZ(), 0.0f, 0.0f);
            mTerrainGPUShader.mShader.SetValue(mShaderWorldSizeHandle, WorldSize);

            Vector4 SkirtVals = new Vector4(TerrainGlobals.getTerrain().getNumSkirtXVertsPerQuadrant(), TerrainGlobals.getTerrain().getNumSkirtZVertsPerQuadrant(), mSkirtTextureSizeX, mSkirtTextureSizeY);
            mTerrainGPUShader.mShader.SetValue(mShaderSkirtValsHandle, SkirtVals);

            if (quadrant == -1)
            {
               mTerrainGPUShader.mShader.SetValue(mShaderSkirtNodeEnabledHandle, false);
               mTerrainGPUShader.mShader.SetValue(mShaderSkirtTexHandle, (BaseTexture)null);
            }
            else
            {
               mTerrainGPUShader.mShader.SetValue(mShaderSkirtNodeEnabledHandle, true);
               mTerrainGPUShader.mShader.SetValue(mShaderSkirtTexHandle, mSkirtTexture[quadrant]);
            }


            //GRID
            mTerrainGPUShader.mShader.SetValue(mShaderGridToggleHandle, mbShowGrid);
            mTerrainGPUShader.mShader.SetValue(mShaderGridSpacingHandle, mGridSpacing);
            mTerrainGPUShader.mShader.SetValue(mShaderGridColor0Handle, mGridColor0);
            mTerrainGPUShader.mShader.SetValue(mShaderGridColor1Handle, mGridColor1);
            mTerrainGPUShader.mShader.SetValue(mShaderGridColor2Handle, mGridColor2);
            mTerrainGPUShader.mShader.SetValue(mShaderGridColor3Handle, mGridColor3);


            if(TerrainGlobals.getEditor().doShowLighting())
            {
               mLightDir = new Vector4(1, 1, 1 , 0);
               mAmbientLight = 0.0f;
            }
            else
            {
               mLightDir = new Vector4(0, 1, 0, 0);
               mAmbientLight = 0.5f;
            }
            mTerrainGPUShader.mShader.SetValue(mShaderDirLightHandle, mLightDir);
            mTerrainGPUShader.mShader.SetValue(mShaderAmbientLightHandle, mAmbientLight);
            

            VertexBuffer vb = null;
            uint numVerts = 0;

            //set LOD Level
            mTerrainGPUShader.mShader.SetValue(mShaderChunkScaleHandle, TerrainGlobals.getTerrain().getTileScale());
            TerrainGlobals.getVisual().getLODVB(ref vb, ref numVerts, (BTerrainVisual.eLODLevel)handle.LOD);
            numVerts = (uint)((numVerts) / 3);
            BRenderDevice.getDevice().SetStreamSource(0, vb, 0);
            float scale = TerrainGlobals.getTerrain().getTileScale() * ((int)Math.Pow(2, handle.LOD));

            Vector4 DataVals = new Vector4(2 * (BTerrainQuadNode.getMaxNodeWidth() >> handle.LOD), scale, handle.minX, handle.minZ);
            mTerrainGPUShader.mShader.SetValue(mShaderDataValsHandle, DataVals);


            int passCount = mTerrainGPUShader.mShader.Begin(FX.DoNotSaveState);
            mTerrainGPUShader.mShader.BeginPass((int)mPassNum);

            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Microsoft.DirectX.Direct3D.Blend.SourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Microsoft.DirectX.Direct3D.Blend.InvSourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaTestEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaFunction, (int)Compare.Greater);
            BRenderDevice.getDevice().SetRenderState(RenderStates.ReferenceAlpha, (int)0x00);

            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, (int)numVerts);

            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaTestEnable, false);

            mTerrainGPUShader.mShader.EndPass();


            mTerrainGPUShader.mShader.End();
         }
         catch (Microsoft.DirectX.Direct3D.InvalidCallException e)
         {
            MessageBox.Show("shit");
         }
      }

   

      public Vector4 mCursorColorTint = new Vector4(1, 1, 1, 1);
      public void renderCursor(BTerrainVisualDataHandle handle)
      {
         if (handle.mHandle == null)
            return;
         try
         {
            Vector3 vev = TerrainGlobals.getEditor().mBrushIntersectionPoint;
            Vector4 shaderIntPt = new Vector4(vev.X, vev.Y, vev.Z, 0);


            BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;

            //SET UP VARIABLES
            Matrix g_matView;
            Matrix g_matProj;
            g_matView = BRenderDevice.getDevice().Transform.View;
            g_matProj = BRenderDevice.getDevice().Transform.Projection;
            Matrix worldViewProjection = g_matView * g_matProj;

            mTerrainGPUCursorShader.mShader.SetValue(mCursorShaderWVPHandle, worldViewProjection);

            //Our position texture
            mTerrainGPUCursorShader.mShader.SetValue(mCursorShaderPositionsTexHandle, handle.mHandle.mPositionsTexture);
            mTerrainGPUCursorShader.mShader.SetValue(mCursorShaderNormalsTexHandle, handle.mHandle.mNormalsTexture);


            float scale = TerrainGlobals.getTerrain().getTileScale() * ((int)Math.Pow(2, handle.LOD));
            Vector4 DataVals = new Vector4(2 * (BTerrainQuadNode.getMaxNodeWidth() >> handle.LOD), scale, handle.minX, handle.minZ);
            mTerrainGPUCursorShader.mShader.SetValue(mCursorShaderDataValsHandle, DataVals);
            mTerrainGPUCursorShader.mShader.SetValue(mCursorChunkScaleHandle, TerrainGlobals.getTerrain().getTileScale());



            //specific to cursor rendering
            Terrain.BrushInfo bi = TerrainGlobals.getEditor().getCurrentBrushInfo();
            Vector4 brushInfo = new Vector4(bi.mRadius * 2,
                                            (bi.mHotspot * bi.mRadius * 2),
                                             0, 0);

            VertexBuffer vb = null;
            uint numVerts = 0;
            int passNum = 0;

            Vector4 selectionColor = new Vector4(1, 1, 0, 1);
            if (TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeTexEdit)
            {
               TerrainGlobals.getVisual().getLODVB(ref vb, ref numVerts, (BTerrainVisual.eLODLevel)handle.LOD);
               numVerts = (uint)((numVerts) / 3);

               mTerrainGPUCursorShader.mShader.SetValue(mCursorMaskHandle, TerrainGlobals.getTerrainFrontEnd().getSelectedMaskTexture());
               passNum = 7;

               if (!TerrainGlobals.getEditor().isInMaskSelectionMode())
               {
                  //set our parameters
                  mTerrainGPUShader.mShader.SetValue(mShaderAtlasScale, 1);

                  BTerrainActiveTextureContainer activeTex = TerrainGlobals.getTexturing().getActiveTexture(TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex);
                  brushInfo.Y = TerrainGlobals.getEditor().mBrushInfo.mIntensity;
                  brushInfo.Z = activeTex.mUScale;
                  brushInfo.W = activeTex.mVScale;

                  //store rotation here..
                  shaderIntPt.W = TerrainGlobals.getEditor().mBrushInfo.mRotation;

                  mTerrainGPUCursorShader.mShader.SetValue(mCursorColorHandle, mCursorColorTint);
                  mTerrainGPUCursorShader.mShader.SetValue(mCursorTexArrayHandle, activeTex.mTexChannels[0].mTexture);
                  passNum = 6;

               }
               else
               {
                  mTerrainGPUCursorShader.mShader.SetValue(mCursorColorHandle, selectionColor);
               }



            }
            else if (TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeDecalEdit)
            {
               TerrainGlobals.getVisual().getLODVB(ref vb, ref numVerts, (BTerrainVisual.eLODLevel)handle.LOD);
               numVerts = (uint)((numVerts) / 3);

               mTerrainGPUShader.mShader.SetValue(mShaderAtlasScale, 1);

               BTerrainActiveDecalContainer activeTex = TerrainGlobals.getTexturing().getActiveDecal(TerrainGlobals.getTerrainFrontEnd().SelectedDecalIndex);
               mTerrainGPUCursorShader.mShader.SetValue(mCursorTexDecalHandle, activeTex.mTexChannels[(int)BTerrainTexturing.eTextureChannels.cAlbedo].mTexture);
               mTerrainGPUCursorShader.mShader.SetValue(mCursorMaskHandle, activeTex.mTexChannels[(int)BTerrainTexturing.eTextureChannels.cOpacity].mTexture);
               brushInfo.Y = TerrainGlobals.getEditor().mBrushInfo.mIntensity;
               brushInfo.Z = TerrainGlobals.getEditor().mBrushInfo.mRotation;
               brushInfo.W = 1;

               if (TerrainGlobals.getEditor().isInMaskSelectionMode())
                  mTerrainGPUCursorShader.mShader.SetValue(mCursorColorHandle, selectionColor);
               else
                  mTerrainGPUCursorShader.mShader.SetValue(mCursorColorHandle, mCursorColorTint);

               passNum = 8;


            }
            else
            {
               //if(TerrainGlobals.getEditor().getCurrentBrush().getIntersectionType() == BTerrainBrush.ValidIntersectionShapes.Sphere)
               if (bi.mIntersectionShape == 0)
                  passNum = bi.mCurveType;
               else
                  passNum = 3 + bi.mCurveType;
               float ptSize = 3;
               BRenderDevice.getDevice().SetRenderState(RenderStates.PointSize, ptSize);
               TerrainGlobals.getVisual().getCursorLODVB(ref vb, ref numVerts, (BTerrainVisual.eLODLevel)handle.LOD);
            }

            mTerrainGPUCursorShader.mShader.SetValue(mCursorShaderBrushHandle, brushInfo);
            mTerrainGPUCursorShader.mShader.SetValue(mCursorShaderInterPointHandle, shaderIntPt);


            BRenderDevice.getDevice().SetStreamSource(0, vb, 0);



            int passCount = mTerrainGPUCursorShader.mShader.Begin(FX.DoNotSaveState);
            mTerrainGPUCursorShader.mShader.BeginPass((int)passNum);
            if (TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeTexEdit ||
                TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeDecalEdit)
            {
               BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, (int)numVerts);
            }
            else
            {
               BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.PointList, 0, (int)numVerts);
            }

            mTerrainGPUCursorShader.mShader.EndPass();


            mTerrainGPUCursorShader.mShader.End();
         }
         catch (Microsoft.DirectX.Direct3D.InvalidCallException e)
         {
            MessageBox.Show("blah");
         }
      }

      public void renderWidget(BTerrainVisualDataHandle handle, int passNum)
      {
         if (handle.mHandle == null)
            return;
         try
         {

            BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;

            //SET UP VARIABLES
            Matrix g_matView;
            Matrix g_matProj;
            g_matView = BRenderDevice.getDevice().Transform.View;
            g_matProj = BRenderDevice.getDevice().Transform.Projection;
            Matrix worldViewProjection = g_matView * g_matProj;

            mTerrainGPUWidgetShader.mShader.SetValue(mWidgetShaderWVPHandle, worldViewProjection);

            //Our position texture
            mTerrainGPUWidgetShader.mShader.SetValue(mWidgetShaderPositionsTexHandle, handle.mHandle.mPositionsTexture);
            mTerrainGPUWidgetShader.mShader.SetValue(mWidgetShaderNormalsTexHandle, handle.mHandle.mNormalsTexture);


            float scale = TerrainGlobals.getTerrain().getTileScale() * ((int)Math.Pow(2, handle.LOD));
            Vector4 DataVals = new Vector4(2 * (BTerrainQuadNode.getMaxNodeWidth() >> handle.LOD), scale, handle.minX, handle.minZ);
            mTerrainGPUWidgetShader.mShader.SetValue(mWidgetShaderDataValsHandle, DataVals);
            mTerrainGPUWidgetShader.mShader.SetValue(mWidgetChunkScaleHandle, TerrainGlobals.getTerrain().getTileScale());

            VertexBuffer vb = null;
            uint numVerts = 0;

            TerrainGlobals.getVisual().getLODVB(ref vb, ref numVerts, (BTerrainVisual.eLODLevel)handle.LOD);
            numVerts = (uint)((numVerts) / 3);
            BRenderDevice.getDevice().SetStreamSource(0, vb, 0);
            int passCount = mTerrainGPUWidgetShader.mShader.Begin(FX.DoNotSaveState);
            mTerrainGPUWidgetShader.mShader.BeginPass((int)passNum);

            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, (int)numVerts);
             
             //  BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.PointList, 0, (int)numVerts);
            
            mTerrainGPUWidgetShader.mShader.EndPass();


            mTerrainGPUWidgetShader.mShader.End();
         }
         catch (Microsoft.DirectX.Direct3D.InvalidCallException e)
         {
            MessageBox.Show("blah");
         }
         
      }


      public void renderSkirtOffsets(int quadrant, Vector4 skirtColor)
      {
         try
         {
            float world_sizeX = (float)TerrainGlobals.getTerrain().getWorldSizeX();
            float world_sizeZ = (float)TerrainGlobals.getTerrain().getWorldSizeZ();


            Matrix world = Matrix.Identity;


            switch (quadrant)
            {
               case 0:
                  world.M11 = -1;
                  world.M33 = -1;
                  break;

               case 1:
                  world.M11 = -1;
                  break;

               case 2:
                  world.M11 = -1;
                  world.M33 = -1;
                  world.M43 = TerrainGlobals.getTerrain().getWorldSizeZ() * 2;
                  break;

               case 3:
                  world.M33 = -1;
                  world.M43 = TerrainGlobals.getTerrain().getWorldSizeZ() * 2;
                  break;

               case 4:
                  world.M11 = -1;
                  world.M33 = -1;
                  world.M41 = TerrainGlobals.getTerrain().getWorldSizeX() * 2;
                  world.M43 = TerrainGlobals.getTerrain().getWorldSizeZ() * 2;
                  break;

               case 5:
                  world.M11 = -1;
                  world.M41 = TerrainGlobals.getTerrain().getWorldSizeX() * 2;
                  break;

               case 6:
                  world.M11 = -1;
                  world.M33 = -1;
                  world.M41 = TerrainGlobals.getTerrain().getWorldSizeX() * 2;
                  break;

               case 7:
               default:
                  world.M33 = -1;
                  break;
            }


            //VP Matrix
            Matrix g_matView;
            Matrix g_matProj;
            g_matView = BRenderDevice.getDevice().Transform.View;
            g_matProj = BRenderDevice.getDevice().Transform.Projection;
            Matrix worldViewProjection = world * g_matView * g_matProj;
            mTerrainGPUShader.mShader.SetValue(mShaderWHandle, world);
            mTerrainGPUShader.mShader.SetValue(mShaderWVPHandle, worldViewProjection);

            //SET UP VARIABLES
            Vector4 WorldSize = new Vector4(TerrainGlobals.getTerrain().getWorldSizeX(), TerrainGlobals.getTerrain().getWorldSizeZ(), 0.0f, 0.0f);
            mTerrainGPUShader.mShader.SetValue(mShaderWorldSizeHandle, WorldSize);

            Vector4 SkirtVals = new Vector4(TerrainGlobals.getTerrain().getNumSkirtXVertsPerQuadrant(), TerrainGlobals.getTerrain().getNumSkirtZVertsPerQuadrant(), mSkirtTextureSizeX, mSkirtTextureSizeY);
            mTerrainGPUShader.mShader.SetValue(mShaderSkirtValsHandle, SkirtVals);

            //skirt texture
            mTerrainGPUShader.mShader.SetValue(mShaderSkirtTexHandle, mSkirtTexture[quadrant]);
            
            //skirt color
            mTerrainGPUShader.mShader.SetValue(mSkirtWireColorHandle, skirtColor);

            BRenderDevice.getDevice().SetStreamSource(0, mSkirtOffsetVertexBuffer, 0);


            int passCount = mTerrainGPUShader.mShader.Begin(FX.DoNotSaveState);
            mTerrainGPUShader.mShader.BeginPass((int)mPassNum);
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineList, 0, (int)mNumSkirtOffsetVerts / 2);
            mTerrainGPUShader.mShader.EndPass();

            mTerrainGPUShader.mShader.End();
         }
         catch (Microsoft.DirectX.Direct3D.InvalidCallException e)
         {
            MessageBox.Show("shit");
         }
      }

      public enum eRenderPass
      {
         eRP_Full                = 0,
         eRP_NoTexturing         = 1,
         eRP_Select_Textured     = 2,
         eRP_Select_NoTextured   = 3,
         eRP_TextureEval         = 4,
         eRP_Normals             = 5,
         eRP_AO                  = 6,
         eRP_SkirtWireframe      = 7,
         eRP_ChunkPerf           = 8,
         eRP_DepthOnly           = 9, //used for XTH export (& potentially shadow maps?)
         eRP_DepthPeel           = 10, //used for AO Gen (& potentially GI later?)

      }
      public void setRenderPass(eRenderPass passNum)
      {
         mPassNum = (uint)passNum;
      }


      public void updateAllSkirtTextures()
      {
         for (int i = 0; i < 8; i++)
         {
            updateSkirtTexture(i);
         }
      }

      public void updateSkirtTexture(int quadrant)
      {
         if (quadrant < 0 || quadrant > 8)
            return;

         int quadrant_width = TerrainGlobals.getTerrain().getNumSkirtXVertsPerQuadrant() - 1;
         int quadrant_height = TerrainGlobals.getTerrain().getNumSkirtZVertsPerQuadrant() - 1;

         int startX, startZ;
         bool invertX, invertZ;
         switch (quadrant)
         {
            case 0:
               startX = quadrant_width * 0;
               startZ = quadrant_height * 0;
               invertX = true;
               invertZ = true;
               break;

            case 1:
               startX = quadrant_width * 0;
               startZ = quadrant_height * 1;
               invertX = true;
               invertZ = false;
               break;

            case 2:
               startX = quadrant_width * 0;
               startZ = quadrant_height * 2;
               invertX = true;
               invertZ = true;
               break;

            case 3:
               startX = quadrant_width * 1;
               startZ = quadrant_height * 2;
               invertX = false;
               invertZ = true;
               break;

            case 4:
               startX = quadrant_width * 2;
               startZ = quadrant_height * 2;
               invertX = true;
               invertZ = true;
               break;

            case 5:
               startX = quadrant_width * 2;
               startZ = quadrant_height * 1;
               invertX = true;
               invertZ = false;
               break;

            case 6:
               startX = quadrant_width * 2;
               startZ = quadrant_height * 0;
               invertX = true;
               invertZ = true;
               break;

            case 7:
            default:
               startX = quadrant_width * 1;
               startZ = quadrant_height * 0;
               invertX = false;
               invertZ = true;
               break;
         }



         int numXVerts = TerrainGlobals.getTerrain().getNumSkirtXVertsPerQuadrant();
         int numZVerts = TerrainGlobals.getTerrain().getNumSkirtZVertsPerQuadrant();

         Texture texture = mSkirtTexture[quadrant];

         unsafe
         {
            GraphicsStream streamPos = texture.LockRectangle(0, LockFlags.Discard);

            float* pos = (float*)streamPos.InternalDataPointer;
            {
               // Clear first
               for (int i = 0; i < mSkirtTextureSizeY * mSkirtTextureSizeX * 4; i++)
               {
                  pos[i] = 0.0f;
               }

               for (int z = 0; z < numZVerts; z++)
               {
                  for (int x = 0; x < numXVerts; x++)
                  {
                     int index = (z * mSkirtTextureSizeX + x) * 4;
                     int xVal = (int)(startX + (invertX ? quadrant_width - x : x));
                     int zVal = (int)(startZ + (invertZ ? quadrant_height - z : z));

                     float skirtHeight1, skirtHeight2, skirtHeight3, skirtHeight4;
                     skirtHeight1 = TerrainGlobals.getTerrain().getSkirtHeight(xVal, zVal);

                     if (invertX && !invertZ)
                     {
                        skirtHeight2 = TerrainGlobals.getTerrain().getSkirtHeight(xVal - 1, zVal);
                        skirtHeight3 = TerrainGlobals.getTerrain().getSkirtHeight(xVal, zVal + 1);
                        skirtHeight4 = TerrainGlobals.getTerrain().getSkirtHeight(xVal - 1, zVal + 1);
                     }
                     else if (!invertX && invertZ)
                     {
                        skirtHeight2 = TerrainGlobals.getTerrain().getSkirtHeight(xVal + 1, zVal);
                        skirtHeight3 = TerrainGlobals.getTerrain().getSkirtHeight(xVal, zVal - 1);
                        skirtHeight4 = TerrainGlobals.getTerrain().getSkirtHeight(xVal + 1, zVal - 1);
                     }
                     else if (invertX && invertZ)
                     {
                        skirtHeight2 = TerrainGlobals.getTerrain().getSkirtHeight(xVal - 1, zVal);
                        skirtHeight3 = TerrainGlobals.getTerrain().getSkirtHeight(xVal, zVal - 1);
                        skirtHeight4 = TerrainGlobals.getTerrain().getSkirtHeight(xVal - 1, zVal - 1);
                     }
                     else //if (!invertX && !invertZ)
                     {
                        skirtHeight2 = TerrainGlobals.getTerrain().getSkirtHeight(xVal + 1, zVal);
                        skirtHeight3 = TerrainGlobals.getTerrain().getSkirtHeight(xVal, zVal + 1);
                        skirtHeight4 = TerrainGlobals.getTerrain().getSkirtHeight(xVal + 1, zVal + 1);
                     }

                     pos[index] = skirtHeight1;
                     pos[index + 1] = skirtHeight2;
                     pos[index + 2] = skirtHeight3;
                     pos[index + 3] = skirtHeight4;
                  }
               }
               texture.UnlockRectangle(0);
            }
         }

      }

      //creation
      private void makeSkirtOffsetsVB()
      {
         VertexTypes.Pos[] verts = null;

         uint width = (uint)TerrainGlobals.getTerrain().getNumSkirtXVertsPerQuadrant();
         uint height = (uint)TerrainGlobals.getTerrain().getNumSkirtZVertsPerQuadrant();
         int counter = 0;


         mNumSkirtOffsetVerts = (width - 1) * (height - 1) * 4;   //*2 (VERTSPERLINE) *2 (PERTILE)
         mNumSkirtOffsetVerts += (((width - 1) + (height - 1)) * 2);
         verts = new VertexTypes.Pos[mNumSkirtOffsetVerts];

         int x, z;
         for (z = 0; z < height - 1; z++)
         {
            for (x = 0; x < width - 1; x++)
            {
               int k = z + (int)(x * height);
               verts[counter++].x = (k);
               verts[counter++].x = (k + 1);

               verts[counter++].x = (k);
               verts[counter++].x = (k + height);
            }
         }

         x = (int)width - 1;
         for (z = 0; z < height - 1; z++)
         {
            int k = z + (int)(x * height);
            verts[counter++].x = (k);
            verts[counter++].x = (k + 1);
         }

         z = (int)height - 1;
         for (x = 0; x < width - 1; x++)
         {
            int k = z + (int)(x * height);
            verts[counter++].x = (k);
            verts[counter++].x = (k + height);
         }
         //oops !! throw new System.Exception("test batch exporter exception");
         mSkirtOffsetVertexBuffer = new VertexBuffer(typeof(VertexTypes.Pos), (int)mNumSkirtOffsetVerts, BRenderDevice.getDevice(), Usage.None, VertexFormats.Position, Pool.Managed);

         GraphicsStream stream = mSkirtOffsetVertexBuffer.Lock(0, (int)mNumSkirtOffsetVerts * sizeof(float) * 3, LockFlags.None);
         stream.Write(verts);
         mSkirtOffsetVertexBuffer.Unlock();


         verts = null;
      }


      /// -----------------------------------------MEMBERS

      public bool mbShowGrid = false;
      public float mGridSpacing = 1.0f;
      public Vector4 mGridColor0 = new Vector4(1, 1, 1, 1);
      public Vector4 mGridColor1 = new Vector4(1, 1, 1, 1);
      public Vector4 mGridColor2 = new Vector4(1, 1, 1, 1);
      public Vector4 mGridColor3 = new Vector4(0, 0, 1, 1);

      public ShaderHandle mTerrainGPUShader = null;


      //ugg.. i hate duplicating this.. but it does speed things up...(i think...)

      public EffectHandle mShaderPositionsTexHandle;
      public EffectHandle mShaderNormalsTexHandle;
      public EffectHandle mShaderAlphasTexHandle;
      public EffectHandle mShaderLightsTexHandle;
      public EffectHandle mShaderTessellationTexHandle;
      
      public EffectHandle mShaderSkirtTexHandle;

      public EffectHandle mShaderWHandle;
      public EffectHandle mShaderWVPHandle;
      public EffectHandle mShaderDataValsHandle;
      public EffectHandle mShaderDataValsHandle2;

      public EffectHandle mShaderSkirtNodeEnabledHandle;
      public EffectHandle mShaderWorldSizeHandle;
      public EffectHandle mShaderSkirtValsHandle;

      public EffectHandle mShaderGridToggleHandle;
      public EffectHandle mShaderGridSpacingHandle;
      public EffectHandle mShaderGridColor0Handle;
      public EffectHandle mShaderGridColor1Handle;
      public EffectHandle mShaderGridColor2Handle;
      public EffectHandle mShaderGridColor3Handle;
      

      //texturing
      public EffectHandle mShaderNumTexHandle;
      public EffectHandle mShaderBlendsIndxHandle;
      public EffectHandle mShaderAlbedoHandle;
      public EffectHandle mShaderNormalHandle;
      public EffectHandle mShaderAlphaTexHandle;
      public EffectHandle mShaderNumBlendsHandle;
      public EffectHandle mShaderRCPNumBlendsHandle;
      public EffectHandle mShaderIndexTexHandle;
      public EffectHandle mShaderChunkScaleHandle;
      public EffectHandle mShaderDirLightHandle;
      public EffectHandle mShaderAmbientLightHandle;
      public EffectHandle mShaderTexUVOffset;
      public EffectHandle mShaderAtlasScale;


      //Cursor Rendering
      public ShaderHandle mTerrainGPUCursorShader = null;
      public EffectHandle mCursorShaderPositionsTexHandle;
      public EffectHandle mCursorShaderNormalsTexHandle;


      public EffectHandle mCursorShaderWVPHandle;
      public EffectHandle mCursorShaderDataValsHandle;
      public EffectHandle mCursorShaderDataValsHandle2;
      public EffectHandle mCursorShaderBrushHandle;
      public EffectHandle mCursorShaderInterPointHandle;
      public EffectHandle mCursorMaskHandle;
      public EffectHandle mCursorTexArrayHandle;
      public EffectHandle mCursorTexDecalHandle;
      public EffectHandle mCursorColorHandle;
      public EffectHandle mCursorChunkScaleHandle;

      //Widget Rendering
      public ShaderHandle mTerrainGPUWidgetShader = null;
      public EffectHandle mWidgetShaderPositionsTexHandle;
      public EffectHandle mWidgetShaderNormalsTexHandle;
      public EffectHandle mWidgetShaderWVPHandle;
      public EffectHandle mWidgetShaderDataValsHandle;
      public EffectHandle mWidgetShaderDataValsHandle2;

      public EffectHandle mWidgetChunkScaleHandle;



      //Misc
      public EffectHandle mWorldCameraPosHandle;

      public EffectHandle mPlanarFogEnabledHandle;
      public EffectHandle mRadialFogColorHandle;
      public EffectHandle mPlanarFogColorHandle;
      public EffectHandle mFogParamsHandle;

      public EffectHandle mSkirtWireColorHandle;

      public EffectHandle mPerfColor;


      public uint mPassNum;


      public Texture[] mSkirtTexture = new Texture[8];
      public int mSkirtTextureSizeX;
      public int mSkirtTextureSizeY;

      private VertexBuffer mSkirtOffsetVertexBuffer = null;
      private uint mNumSkirtOffsetVerts = 0;
   }
   #endregion
}
