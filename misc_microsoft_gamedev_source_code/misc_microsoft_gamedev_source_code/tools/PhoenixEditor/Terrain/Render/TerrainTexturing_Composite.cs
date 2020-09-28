using System;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;

using Rendering;
using EditorCore;
using SimEditor;


 
//-----------------------------------------------
namespace Terrain
{
   //------------------------------------------------------------
   //these are used by the terrain, layers are composted to this, and this texture is used in the final shader
   public enum eCompositeTextureState
   {
      cCS_Free = 0,
      cCS_Used,
      cCS_UsedThisFrame
   };

   public class BTerrainCompositeAtlasTexture
   {
      public BTerrainCompositeAtlasTexture(int width, int height, int numEntries, int rows, int columns, int numMips, Format fmt)
      {
         create(width, height, numEntries, rows, columns, numMips, fmt);
      }

      public void reCreate()
      {
         mTextures = new Texture[(int)BTerrainTexturing.eTextureChannels.cSplatChannelCount];
         for (int i = 0; i < (int)BTerrainTexturing.eTextureChannels.cSplatChannelCount; i++)
         {
            mTextures[i] = new Texture(BRenderDevice.getDevice(), mTotalWidth, mTotalHeight, mNumMips, Usage.RenderTarget, mFmt, Pool.Default);
         }
      }

      public void create(int width, int height, int numEntries, int rows, int columns, int numMips, Format fmt)
      {
         if (numMips == 0)
            numMips = 1;
         mWidth = width;
         mHeight = height;
         mNumMips = numMips;
         mNumEntries = numEntries;
         mRows = rows;
         mColumns = columns;
         mFmt = fmt;

         mTotalHeight = mRows * mHeight;
         mTotalWidth = mColumns * mWidth;


         mAtlasScale = mWidth / (float)mTotalWidth;

         mTextures = new Texture[(int)BTerrainTexturing.eTextureChannels.cSplatChannelCount];
         for (int i = 0; i < (int)BTerrainTexturing.eTextureChannels.cSplatChannelCount; i++)
         {
            //mTextures[i] = new Texture(BRenderDevice.getDevice(), mTotalWidth, mTotalHeight, numMips, Usage.RenderTarget, Format.A8R8G8B8, Pool.Default);
            // mTextures[i] = new Texture(BRenderDevice.getDevice(), mTotalWidth, mTotalHeight, numMips, Usage.RenderTarget, Format.R5G6B5, Pool.Default);
            mTextures[i] = new Texture(BRenderDevice.getDevice(), mTotalWidth, mTotalHeight, numMips, Usage.RenderTarget, fmt, Pool.Default);
         }

      }
      public void destroy()
      {
         if (mTextures != null)
         {
            for (int i = 0; i < (int)BTerrainTexturing.eTextureChannels.cSplatChannelCount; i++)
            {
               if (mTextures[i] != null)
               {
                  mTextures[i].Dispose();
                  //mTextures[i] = null;
               }
            }
            mTextures = null;
         }
      }

      public void getOffsetInfo(int index, out Vector2 uvOffset, out int xpixel, out int ypixel)
      {
         //               long index = c + TerrainGlobals.getTerrain().getNumXVerts() * r;

         int columnID = index / mRows;
         int rowID = index - columnID * mRows;

         uvOffset.X = (float)columnID / (float)mColumns;
         uvOffset.Y = (float)rowID / (float)mRows;

         xpixel = columnID * mWidth;
         ypixel = rowID * mHeight;

      }


      public Texture[] mTextures;

      public int mWidth;
      public int mHeight;
      public int mTotalWidth;
      public int mTotalHeight;
      public int mRows;
      public int mColumns;
      public int mNumEntries;
      public int mNumMips;
      public Format mFmt;
      public float mAtlasScale;

   }

   public class BTerrainCompositeTexture
   {

      public BTerrainCompositeTexture()
      {

      }
      public BTerrainCompositeTexture(int width, int height, int numMips, BTerrainCompositeAtlasTexture atlas, int index)
      {
         create(width, height, numMips, atlas, index);
      }
      public void create(int width, int height, int numMips, BTerrainCompositeAtlasTexture atlas, int index)
      {
         mAtlas = atlas;

         if (numMips == 0)
            numMips = 1;

         mWidth = width;
         mHeight = height;
         mNumMips = numMips;
         mIndexInCache = index;

         if (mAtlas == null)
         {
            mTextures = new Texture[(int)BTerrainTexturing.eTextureChannels.cSplatChannelCount];
            for (int i = 0; i < (int)BTerrainTexturing.eTextureChannels.cSplatChannelCount; i++)
               mTextures[i] = new Texture(BRenderDevice.getDevice(), width, height, numMips, Usage.RenderTarget, Format.R5G6B5, Pool.Default);

         }
         else
         {
            mAtlas.getOffsetInfo(index, out mAtlasUVOffset, out mXPixelOffset, out mYPixelOffset);
         }
      }

      public int mXPixelOffset = 0;
      public int mYPixelOffset = 0;

      public bool UsingAtlas
      {
         get
         {
            if (mAtlas != null)
               return true;
            return false;
         }
      }

      public void destroy()
      {
         if (mTextures != null)
         {
            for (int i = 0; i < (int)BTerrainTexturing.eTextureChannels.cSplatChannelCount; i++)
            {
               if (mTextures[i] != null)
               {
                  mTextures[i].Dispose();
                  mTextures[i] = null;
               }
            }
            mTextures = null;
         }

      }


      public void decouple()
      {
         if (mOwnerNode != null)
         {
            //mOwnerNode.getTextureData().mCompositeTexture = null;

            int lod = BTerrainTexturing.widthToLOD(mWidth);
            if (mOwnerNode.getTextureData(lod) != null)
               mOwnerNode.getTextureData(lod).mCompositeTexture = null;

            mOwnerNode = null;
         }
      }
      public void free()
      {
         mState = eCompositeTextureState.cCS_Free;
         decouple();
      }
      public Texture[] mTextures = null;

      public eCompositeTextureState mState = eCompositeTextureState.cCS_Free;
      public int mWidth;
      public int mHeight;
      public int mNumMips;
      public BTerrainQuadNode mOwnerNode = null;

      public BTerrainCompositeAtlasTexture mAtlas = null;

      public int mIndexInCache;
      public Vector2 mAtlasUVOffset;
   }
   public class BTerrainCompositeCache
   {
      public void create(int width, int height, int startingReserve)
      {
         mWidth = width;
         mHeight = height;
         int numMips = (mWidth < BTerrainTexturing.getTextureHeight() >> 1) ? 1 : BTerrainTexturing.cMaxNumMips;
         mCache = new List<BTerrainCompositeTexture>(startingReserve);


         if (startingReserve >= 2)//256) //use atlas
         {
            mAtlasList = new List<BTerrainCompositeAtlasTexture>();

            int atlasSize = 1024;

            int maxColumns = atlasSize / mWidth;
            int maxRows = atlasSize / mHeight;
            int maxEntries = maxColumns * maxRows;

            while (maxEntries / startingReserve >= 2)
            {
               atlasSize /= 2;
               maxColumns = atlasSize / mWidth;
               maxRows = atlasSize / mHeight;
               maxEntries = maxColumns * maxRows;
            }

            int neededAtlases = (int)Math.Ceiling(startingReserve / (float)maxEntries);

            int currentIndex = 0;
            BTerrainCompositeAtlasTexture currentAtlas = new BTerrainCompositeAtlasTexture(mWidth, mHeight, maxEntries, maxRows, maxColumns, numMips, Format.R5G6B5);
            mAtlasList.Add(currentAtlas);
            for (int i = 0; i < startingReserve; i++)
            {

               mCache.Add(new BTerrainCompositeTexture(mWidth, mHeight, numMips, currentAtlas, currentIndex));
               currentIndex++;
               if (currentIndex >= maxEntries && currentIndex < startingReserve)
               {
                  currentAtlas = new BTerrainCompositeAtlasTexture(mWidth, mHeight, maxEntries, maxRows, maxColumns, numMips, Format.R5G6B5);
                  mAtlasList.Add(currentAtlas);
                  currentIndex = 0;
               }

            }
         }
         else //normal way
         {
            for (int i = 0; i < startingReserve; i++)
            {
               mCache.Add(new BTerrainCompositeTexture(mWidth, mHeight, numMips, null, -1));
            }
         }


      }
      public void destroy()
      {
         for (int i = 0; i < mCache.Count; i++)
            mCache[i].destroy();
         mCache.Clear();

         if (mAtlasList != null)
         {

            foreach (BTerrainCompositeAtlasTexture atlas in mAtlasList)
            {
               atlas.destroy();
            }
         }
      }

      public BTerrainCompositeTexture getCompositeTexture()
      {
         for (int i = 0; i < mCache.Count; i++)
         {
            if (mCache[i].mState == eCompositeTextureState.cCS_Free)
            {
               mCache[i].mState = eCompositeTextureState.cCS_Used;
               return mCache[i];
            }
         }

         int lod = 0;
         switch (mWidth)
         {
            case 512: lod = 0; break;
            case 256: lod = 1; break;
            case 128: lod = 2; break;
            case 64: lod = 3; break;
            case 32: lod = 4; break;
            case 16: lod = 5; break;
         }

         //we didn't find a free texture. Grab a used one.
         int lowestCost = 99999999;
         int lowestIndex = -1;
         for (int i = 0; i < mCache.Count; i++)
         {
            if (mCache[i].mState == eCompositeTextureState.cCS_Used)
            {
               int cost = mCache[i].mOwnerNode.evaluateCost(lod);
               if (cost < lowestCost)
               {
                  lowestIndex = i;
                  lowestCost = cost;
               }
            }
         }
         if (lowestIndex != -1)
         {
            mCache[lowestIndex].decouple();
            return mCache[lowestIndex];
         }


         return null;
      }
      public void free()
      {
         for (int i = 0; i < mCache.Count; i++)
         {
            mCache[i].free();
         }
      }

      public int mWidth;
      public int mHeight;

      public List<BTerrainCompositeTexture> mCache;
      public List<BTerrainCompositeAtlasTexture> mAtlasList = null;
   }

   public partial class BTerrainTexturing
   {
      public void initComposite()
      {
         //Load Our Shader
         string errors = "";
         if (mCompositeShader == null || mCompositeShader.Disposed == true)
         {
            try
            {
               //if (File.Exists("shaders\\gpuTerrainEditor.fx"))
               //string filename = CoreGlobals.getWorkPaths().mBaseDirectory + "\\shaders\\gpuCompositeTexture.fx";
               string filename = CoreGlobals.getWorkPaths().mEditorShaderDirectory + "\\gpuCompositeTexture.fx";
               mCompositeShader = Microsoft.DirectX.Direct3D.Effect.FromFile(BRenderDevice.getDevice(), filename, null, null, ShaderFlags.None, null, out errors);
               if (mCompositeShader == null)
               {
                  MessageBox.Show("Shader did not load:\n" + filename + "\n" + errors);
               }
            }
            catch (System.Exception ex)
            {
               MessageBox.Show("Shader did not load");
               throw (ex);
            }
         }

         mShaderTexArrayHandle = mCompositeShader.GetParameter(null, "targetTexture");

         mShaderAlphaTexArrayHandle = mCompositeShader.GetParameter(null, "alphaTexture");

         mShaderTexDecalHandle = mCompositeShader.GetParameter(null, "targetTextureDecal");
         mShaderAlphaTexDecalHandle = mCompositeShader.GetParameter(null, "decalAlphaTexture");

         mShaderNumLayersHandle = mCompositeShader.GetParameter(null, "gTargetAlphaLayer");
         mShaderLayerUV = mCompositeShader.GetParameter(null, "gUVScale");
         mShaderLayerDecalData = mCompositeShader.GetParameter(null, "gDecalInf");

         mShaderColorOverload = mCompositeShader.GetParameter(null, "gColorOverload");
         mShaderContribOverride = mCompositeShader.GetParameter(null, "gContribOverride");
         mShaderAlphaOverride = mCompositeShader.GetParameter(null, "gAlphaOverride");

         const float pixelOfs = .0005f;
         int numVertsPerQuad = 4;

         float minFX = 1.0f / 64.0f;// 128.0f;
         float maxFX = 1.0f - minFX;
         float minFY = 1.0f / 64.0f;// 128.0f;
         float maxFY = 1.0f - minFX;

         VertexTypes.PosW_uv0_uv1[] g_QuadVertices = new VertexTypes.PosW_uv0_uv1[]{
                                    //  x      y      z    w      tu    tv   tu1    tv1
            new VertexTypes.PosW_uv0_uv1(-1.0f, +1.0f, 0.0f, 1.0f,  0.0f, 0.0f,  minFX, minFY) ,
            new VertexTypes.PosW_uv0_uv1(+1.0f, +1.0f, 0.0f, 1.0f,  1.0f, 0.0f,  maxFX, minFY) ,
            new VertexTypes.PosW_uv0_uv1(+1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f,  maxFX, maxFY) ,
            new VertexTypes.PosW_uv0_uv1(-1.0f, -1.0f, 0.0f, 1.0f,  0.0f, 1.0f,  minFX, maxFY) ,
	      };
         mCompositeVertexBuffer = new VertexBuffer(typeof(VertexTypes.PosW_uv0_uv1), numVertsPerQuad, BRenderDevice.getDevice(), 0, VertexTypes.PosW_uv0_uv1.FVF_Flags, Pool.Managed);

         using (GraphicsStream stream = mCompositeVertexBuffer.Lock(0, 0, LockFlags.None))
         {

            stream.Write(g_QuadVertices);

            mCompositeVertexBuffer.Unlock();
         }


         int[] inds = new int[] { 0, 1, 2, 0, 3, 2 };
         mCompositeIndexBuffer = new IndexBuffer(BRenderDevice.getDevice(), sizeof(int) * 6, 0, Pool.Managed, false);
         using (GraphicsStream stream = mCompositeIndexBuffer.Lock(0, 0, LockFlags.None))
         {
            stream.Write(inds);
            mCompositeIndexBuffer.Unlock();
         }

         //depth stencil texture
         mDepthStenSurf = BRenderDevice.getDevice().CreateDepthStencilSurface(1024, 1024, DepthFormat.D24S8, MultiSampleType.None, 0, false);

         for (int i = 0; i < cMaxNumBlends; i++)
         {
            mTempAlphaTextures.Add(new Texture(BRenderDevice.getDevice(), (int)cAlphaTextureWidth, (int)cAlphaTextureHeight, 1, 0, Format.L8, Pool.Managed));
         }

      }

      //CLM USED FOR GAME
      public unsafe void convertLayersToTexturingDataHandle(BTerrainLayerContainer input, BTerrainCompositeTexture output, BTerrainQuadNode node, int lod)
      {

         //lock in our alpha texture
         BTerrainQuadNodeDesc desc = node.getDesc();
         int numlayers = input.getNumLayers();
         BTerrainLayerContainer tContainer = TerrainGlobals.getEditor().generateContainerFromTexDeformations(desc.mMinXVert, desc.mMinZVert);
         if(tContainer == null)
            tContainer = input;


         tContainer.toTextureArray(ref mTempAlphaTextures, desc.mMinXVert, desc.mMinZVert);
         convertLayersToTexturingDataHandle(tContainer, mTempAlphaTextures, output, desc.mMinXVert, desc.mMinZVert, lod, (int)BTerrainTexturing.eTextureChannels.cSplatChannelCount);

         tContainer = null;
      }
      //CLM USED FOR EXPORT
      public unsafe void convertLayersToTexturingDataHandle(BTerrainLayerContainer input, List<Texture> tempAlphaTextures, BTerrainCompositeTexture output, int minXVert, int minZVert, int lod, int channelCount)
      {
         //  BTerrainLayerContainer input = node.mLayerContainer;
         //  BTerrainCompositeTexture output = node.getTextureData(lod).mCompositeTexture;

         Viewport vp = new Viewport();
         vp.X = output.mXPixelOffset;
         vp.Y = output.mYPixelOffset;
         vp.Width = output.mWidth;
         vp.Height = output.mWidth;
         vp.MinZ = 0;
         vp.MaxZ = 1;
         BRenderDevice.getDevice().Viewport = vp;

         Microsoft.DirectX.Direct3D.Effect shader = TerrainGlobals.getTexturing().mCompositeShader;
         shader.Begin(0);
         shader.BeginPass(0);

         for (int i = 0; i < channelCount; i++)
         {
            for (int k = 0; k < output.mNumMips; k++)
            {

               //actually render
               Surface compositeTarget = null;
               float scale = 1;

               try
               {
                  if (output.UsingAtlas == false)
                  {
                     compositeTarget = output.mTextures[i].GetSurfaceLevel(k);
                     BRenderDevice.getDevice().SetRenderTarget(0, compositeTarget);
                  }
                  else
                  {


                     BTerrainCompositeAtlasTexture atlas = output.mAtlas;

                     if (atlas.mTextures[i].Disposed == true)
                     {
                        atlas.reCreate();
                     }

                     scale = atlas.mAtlasScale;
                     compositeTarget = atlas.mTextures[i].GetSurfaceLevel(k);
                     BRenderDevice.getDevice().SetRenderTarget(0, compositeTarget);

                  }


                  {
                     BTerrainTexturingLayer.eLayerType lastType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;

                     float layerInc = 1.0f / (float)(cMaxNumBlends - 1);// (float)input.getNumLayers();
                     for (int j = 0; j < input.getNumLayers(); j++)
                     {
                        //    BRenderDevice.getDevice().Clear(ClearFlags.ZBuffer | ClearFlags.Target, unchecked((int)0xFFFF0000), 1.0f, 0);

                        //compose a splat
                        shader.SetValue(mShaderContribOverride, 1);
                        shader.SetValue(mShaderAlphaOverride, 1);
                        if (input.giveLayer(j).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
                        {

                           if (lastType != BTerrainTexturingLayer.eLayerType.cLayer_Splat)
                           {
                              shader.EndPass();
                              shader.BeginPass(0);
                              lastType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
                           }

                           if (TerrainGlobals.getEditor().getRenderMode() == BTerrainEditor.eEditorRenderMode.cRenderTextureSelectRender)
                           {

                              if (TerrainGlobals.getEditor().getMode() != BTerrainEditor.eEditorMode.cModeTexEdit)
                              {
                                 if (j == 0)
                                 {
                                    shader.SetValue(mShaderContribOverride, 0);
                                    shader.SetValue(mShaderAlphaOverride, 0);
                                 }
                                 else
                                 {
                                    continue;
                                 }
                              }
                              else
                              {
                                 if (input.giveLayer(j).mActiveTextureIndex != TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex)
                                 {
                                    shader.SetValue(mShaderContribOverride, 0);
                                 }
                                 else
                                 {
                                    shader.SetValue(mShaderContribOverride, 1);
                                 }
                                 shader.SetValue(mShaderAlphaOverride, 0);
                              }

                           }


                           float targetLayer = (float)(j * layerInc);
                           shader.SetValue(mShaderNumLayersHandle, targetLayer);
                           shader.SetValue(mShaderAlphaTexArrayHandle, tempAlphaTextures[j]);


                           //lock in our target texture
                           BTerrainActiveTextureContainer active = TerrainGlobals.getTexturing().getActiveTexture(input.giveLayer(j).mActiveTextureIndex);
                           if (active == null)
                           {
                              compositeTarget.Dispose();
                              continue;
                           }

                           shader.SetValue(mShaderTexArrayHandle, active.mTexChannels[i].mTexture);
                           float[] uvs = new float[2];
                           uvs[0] = active.mUScale;
                           uvs[1] = active.mVScale;

                           shader.SetValue(mShaderLayerUV, uvs);
                        }
                        else //compose a decal
                        {

                           if (lastType != BTerrainTexturingLayer.eLayerType.cLayer_Decal)
                           {
                              shader.EndPass();
                              shader.BeginPass(1);
                              lastType = BTerrainTexturingLayer.eLayerType.cLayer_Decal;
                           }

                           bool doWhite = false;
                           if (TerrainGlobals.getEditor().getRenderMode() == BTerrainEditor.eEditorRenderMode.cRenderTextureSelectRender)
                           {
                              if (TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeTexEdit)
                              {
                                 shader.SetValue(mShaderContribOverride, 0);

                              }
                              else if (TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeDecalEdit)
                              {
                                 doWhite = true;
                                 if (TerrainGlobals.getTexturing().getActiveDecalInstance(input.giveLayer(j).mActiveTextureIndex).mActiveDecalIndex != TerrainGlobals.getTerrainFrontEnd().SelectedDecalIndex)
                                 {
                                    shader.SetValue(mShaderContribOverride, 0);
                                 }
                                 else
                                 {
                                    shader.SetValue(mShaderContribOverride, 1);
                                    shader.SetValue(mShaderAlphaOverride, 1);
                                 }
                              }

                           }

                           //Grab our decal instance
                           BTerrainDecalInstance decal = getActiveDecalInstance(input.giveLayer(j).mActiveTextureIndex);
                           if (decal == null)
                           {
                              compositeTarget.Dispose();
                              continue;
                           }
                           Vector4 selColor = new Vector4(1, 1, 1, 1);
                           if (decal.mIsSelected)
                           {
                              selColor.Y = 0.75f;
                              selColor.Z = 0.75f;
                           }
                           shader.SetValue(mShaderColorOverload, selColor);



                           //grab the decal we care about
                           BTerrainActiveDecalContainer active = getActiveDecal(decal.mActiveDecalIndex);
                           if (active == null)
                           {
                              compositeTarget.Dispose();
                              continue;
                           }
                           if (doWhite)
                              shader.SetValue(mShaderAlphaTexArrayHandle, tempAlphaTextures[0]);
                           else
                              shader.SetValue(mShaderAlphaTexArrayHandle, tempAlphaTextures[j]);
                           shader.SetValue(mShaderAlphaTexDecalHandle, active.mTexChannels[(int)BTerrainTexturing.eTextureChannels.cOpacity].mTexture);
                           shader.SetValue(mShaderTexDecalHandle, active.mTexChannels[i].mTexture);

                           float[] decalDat = new float[4];
                           decalDat[0] = decal.mRotation;


                           //compute our U and V offset
                           float vertsToHighResPixelSpaceRatio = BTerrainTexturing.getTextureWidth() / BTerrainQuadNode.cMaxWidth;
                           decalDat[1] = (decal.mTileCenter.X - (minXVert * vertsToHighResPixelSpaceRatio)) / BTerrainTexturing.getTextureWidth();
                           decalDat[2] = (decal.mTileCenter.Y - (minZVert * vertsToHighResPixelSpaceRatio)) / BTerrainTexturing.getTextureHeight();

                           decalDat[3] = 0;

                           shader.SetValue(mShaderLayerDecalData, decalDat);

                           float[] uvs = new float[2];
                           uvs[0] = decal.mUScale;
                           uvs[1] = decal.mVScale;
                           shader.SetValue(mShaderLayerUV, uvs);
                        }

                        shader.CommitChanges();
                        BRenderDevice.getDevice().Viewport = vp;
                        BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, 4, 0, 2);
                     }

                     shader.EndPass();
                     shader.BeginPass(0);
                  }
               }
               catch (Direct3DXException e)
               {
                  CoreGlobals.getErrorManager().SendToErrorWarningViewer("An error has occured during the compositing process");
               }
               finally
               {
                  if (compositeTarget != null)
                     compositeTarget.Dispose();
               }
            }
         }
         shader.EndPass();
         shader.End();



      }

      //Compositing process
      public Microsoft.DirectX.Direct3D.Effect mCompositeShader = null;
      public EffectHandle mShaderTexArrayHandle = null;
      public EffectHandle mShaderTexDecalHandle = null;
      public EffectHandle mShaderAlphaTexDecalHandle = null;
      public EffectHandle mShaderAlphaTexArrayHandle = null;
      public EffectHandle mShaderTexIndexesHandle = null;
      public EffectHandle mShaderNumLayersHandle = null;
      public EffectHandle mShaderLayerUV = null;
      public EffectHandle mShaderLayerDecalData = null;
      public EffectHandle mShaderColorOverload = null;
      public EffectHandle mShaderContribOverride = null;
      public EffectHandle mShaderAlphaOverride = null;
      public List<Texture> mTempAlphaTextures = new List<Texture>();

      //used for composition
      private VertexBuffer mCompositeVertexBuffer = null;
      private IndexBuffer mCompositeIndexBuffer = null;
      private VertexDeclaration mVertexDecl = null;
      Surface mDepthStenSurf = null;
      Viewport savedViewport;
      Surface savedRenderTarget = null;
      Surface savedDepthSurf = null;
   }
}