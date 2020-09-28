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
   #region TerrainTexturing

   //------------------------------------------------------------
   //these are called and used by the sim and the VQ manager
   public class BTerrainPerVertexLayerData
   {
      public int mActiveTextureIndex;
      public byte mAlphaContrib;
      public BTerrainTexturingLayer.eLayerType mLayerType;
   }

   public class BTerrainTextureVector
   {
      public List<BTerrainPerVertexLayerData> mLayers = new List<BTerrainPerVertexLayerData>();

      //Functions which will effect both layer types
      public int getNumLayers()
      {
         return mLayers.Count;
      }
      public void clearAllLayers()
      {
         mLayers.Clear();
      }

      public void initLayer(int layerID, int textureID, byte value, BTerrainTexturingLayer.eLayerType layerType)
      {
         while (mLayers.Count <= layerID)
         {
            mLayers.Add(new BTerrainPerVertexLayerData());
            mLayers[mLayers.Count - 1].mLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
            mLayers[mLayers.Count - 1].mActiveTextureIndex = textureID;
            mLayers[mLayers.Count - 1].mAlphaContrib = 0;
         }
         mLayers[layerID].mAlphaContrib = value;
         mLayers[layerID].mLayerType = layerType;
         mLayers[layerID].mActiveTextureIndex = textureID;        
      }


      public void addValueToTexID(int textureID, byte value, BTerrainTexturingLayer.eLayerType layerType)
      {
         //let's subtract this value from all indexes above us.
         int index = giveLayerIndex(textureID, layerType);
         if (index == -1)
            index = newSplatLayer(textureID);

         mLayers[index].mAlphaContrib = (byte)BMathLib.Clamp(mLayers[index].mAlphaContrib + value, 0, byte.MaxValue);

         for (int i = index + 1; i < mLayers.Count; i++)
         {
            mLayers[i].mAlphaContrib = (byte)BMathLib.Clamp(mLayers[i].mAlphaContrib - value, 0, byte.MaxValue);
         }

      }
      public int giveLayerID(int layerIndex, ref BTerrainTexturingLayer.eLayerType layerType)
      {
         if (layerIndex < 0 || layerIndex >= mLayers.Count)
         {
            layerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
            return -1;
         }

         layerType = mLayers[layerIndex].mLayerType;
         return mLayers[layerIndex].mActiveTextureIndex;
      }
      public int giveLayerIndex(int activeTextureIndex, BTerrainTexturingLayer.eLayerType layerType)
      {

         for (int i = 0; i < mLayers.Count; i++)
         {
            if (mLayers[i].mActiveTextureIndex == activeTextureIndex && mLayers[i].mLayerType == layerType)
            {
               return i;
            }
         }

         //not found;
         return -1;
      }
      public bool containsID(int activeTextureIndex, BTerrainTexturingLayer.eLayerType layerType)
      {
         for (int i = 0; i < mLayers.Count; i++)
         {
            if (mLayers[i].mActiveTextureIndex == activeTextureIndex &&
               mLayers[i].mLayerType == layerType)
               return true;
         }
         return false;
      }

      //splatting only
      public int newSplatLayer(int activeTextureIndex)
      {
         if (mLayers.Count > 0
            && mLayers[mLayers.Count - 1].mActiveTextureIndex == activeTextureIndex
            && mLayers[mLayers.Count - 1].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
            return mLayers.Count - 1;

         BTerrainPerVertexLayerData vld = new BTerrainPerVertexLayerData();
         vld.mLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
         vld.mActiveTextureIndex = activeTextureIndex;
         vld.mAlphaContrib = 0;

         //find where we should be inserted
         for (int i = 0; i < mLayers.Count; i++)
         {
            if (mLayers[i].mActiveTextureIndex > activeTextureIndex)
            {
               mLayers.Insert(i, vld);
               return i;
            }

         }

         //we didn't find a spot.
         mLayers.Add(vld);

         return mLayers.Count - 1;
      }
      public int getNumSplatLayers()
      {
         int count = 0;
         for (int i = 0; i < mLayers.Count; i++)
            if (mLayers[i].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
               count++;
         return count;
      }

      //decals
      public int newDecalLayer(int activeDecalInstanceIndex)
      {
         if (mLayers.Count > 0
               && mLayers[mLayers.Count - 1].mActiveTextureIndex == activeDecalInstanceIndex
               && mLayers[mLayers.Count - 1].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
            return mLayers.Count - 1;

         BTerrainPerVertexLayerData vld = new BTerrainPerVertexLayerData();
         vld.mLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Decal;
         vld.mActiveTextureIndex = activeDecalInstanceIndex;
         vld.mAlphaContrib = 255;

         mLayers.Add(vld);

         return mLayers.Count - 1;
      }
      public int getNumDecalLayers()
      {
         int count = 0;
         for (int i = 0; i < mLayers.Count; i++)
            if (mLayers[i].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
               count++;
         return count;
      }


      public void copyTo(BTerrainTextureVector target)
      {
         target.mLayers.Clear();
         for (int i = 0; i < mLayers.Count; i++)
         {
            BTerrainPerVertexLayerData vld = new BTerrainPerVertexLayerData();
            vld.mActiveTextureIndex = mLayers[i].mActiveTextureIndex;
            vld.mAlphaContrib = mLayers[i].mAlphaContrib;
            vld.mLayerType = mLayers[i].mLayerType;
            target.mLayers.Add(vld);
         }
      }
      public void swapWith(BTerrainTextureVector target)
      {
         BTerrainTextureVector temp = new BTerrainTextureVector();
         copyTo(temp);
         target.copyTo(this);
         temp.copyTo(target);
      }
      public void mergeWith(BTerrainTextureVector target, bool useSrcOnCollision)
      {
         for (int i = 0; i < target.mLayers.Count; i++)
         {
            if (!containsID(target.mLayers[i].mActiveTextureIndex, target.mLayers[i].mLayerType))
            {
               if (target.mLayers[i].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
               {
                  int idx = newSplatLayer(target.mLayers[i].mActiveTextureIndex);
                  mLayers[idx].mAlphaContrib = target.mLayers[i].mAlphaContrib;
               }
               else if (target.mLayers[i].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
               {
                  int idx = newDecalLayer(target.mLayers[i].mActiveTextureIndex);
                  mLayers[idx].mAlphaContrib = target.mLayers[i].mAlphaContrib;
               }
            }
            else
            {
               if (!useSrcOnCollision)
               {
                  int idx = giveLayerIndex(target.mLayers[i].mActiveTextureIndex, target.mLayers[i].mLayerType);
                  mLayers[idx].mAlphaContrib = target.mLayers[i].mAlphaContrib;
               }
            }
         }
      }

      //Helper Converters for Constrained Splatting
      void ensureProperSorting()
      {
         //bubbesort
         for (int k = 0; k < mLayers.Count; k++)
         {
            for (int j = k + 1; j < mLayers.Count; j++)
            {
               if (mLayers[j].mActiveTextureIndex < mLayers[k].mActiveTextureIndex)
               {
                  mLayers.Insert(k, mLayers[j]);
                  mLayers.RemoveAt(j + 1);
               }
            }
         }
      }
      void ensureProperNumberLayers()
      {
         bool[] usedTexSlots = new bool[TerrainGlobals.getTexturing().getActiveTextureCount()];
         for (int k = 0; k < mLayers.Count; k++)
         {
            if (mLayers[k].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
            {
               usedTexSlots[mLayers[k].mActiveTextureIndex] = true;
            }
         }


         for (int k = 0; k < usedTexSlots.Length; k++)
         {
            if (!usedTexSlots[k])
            {
               newSplatLayer(k);
            }
         }
      }
      public void ensureProperLayerOrdering()
      {
         ensureProperNumberLayers();
         int numSplatLayers = TerrainGlobals.getTexturing().getActiveTextureCount();
 
         //create N floatN vectors
         floatN[] layerVecs = new floatN[numSplatLayers];
         for (uint k = 0; k < numSplatLayers; k++)
         {
            layerVecs[k] = new floatN((uint)numSplatLayers);
            layerVecs[k][k] = 1;
         }

         //now, calculate a VContrib vector for the current layering.
         floatN vContrib = layerVecs[0].Clone();//start with 100% base layer
         for (uint k = 1; k < numSplatLayers; k++)
         {
            byte b = mLayers[(int)k].mAlphaContrib;
            int lIdx = mLayers[(int)k].mActiveTextureIndex;
            float alpha = ((float)b) / 255.0f;
            vContrib = (
               (layerVecs[lIdx] * alpha) + (vContrib * (1 - alpha))
               );
         }

         //vContrib now holds at each vector element, the amount the layer is visible to the final pixel
         //although it's not sorted in the current layer ordering
         //so, calculate the xF for the new layer ordering, and store it in a temp var
         floatN vContribSorted = new floatN((uint)numSplatLayers);

         //back solve to calculate new alpha value.
         vContribSorted[(uint)(numSplatLayers - 1)] = vContrib[(uint)(numSplatLayers - 1)];
         for (int k = (numSplatLayers - 2); k >= 0; k--)
         {
            uint invK = (uint)((numSplatLayers - 2) - k);
            float Vc = vContrib[(uint)(k + 1)];   //vContrib for upper layer
            float invVc = 1.0f - Vc;

            //current layer vContrib
            float cVc = vContrib[(uint)(k)];
            float xF = invVc == 0 ? 0 : (cVc / invVc);

            //move back 
            vContribSorted[(uint)k] = xF;
         }

         //now copy back to the unsorted layer index
         for (int k = 0; k < numSplatLayers; k++)
         {
            mLayers[k].mAlphaContrib = (byte)(vContribSorted[(uint)mLayers[k].mActiveTextureIndex] * 255);
         }

         //ensure layers are sorted
         ensureProperSorting();
      }

   }
   //------------------------------------------------------------
   //this is used to keep store of all textures in active set
   public class BTerrainActiveTextureContainer
   {
      public void destroy()
      {
         if (mTexChannels != null)
         {
            for (int i = 0; i < (int)BTerrainTexturing.eTextureChannels.cSplatChannelCount; i++)
            {
               if (mTexChannels[i] != null)
               {
                  BRenderDevice.getTextureManager().freeTexture(mTexChannels[i].mFilename);
                  mTexChannels[i].destroy();
                  mTexChannels[i] = null;
               }
            }
            mTexChannels = null;
         }
         
      }

      public void destroyDeviceData()
      {
         for (int i = 0; i < (int)BTerrainTexturing.eTextureChannels.cSplatChannelCount; i++)
         {
            if (mTexChannels[i] != null)
            {
               mTexChannels[i].destroy();
               BRenderDevice.getTextureManager().freeTexture(mTexChannels[i].mFilename);
            }
         }
      }

      public void loadTextures()
      {
         if (mTexChannels==null)
            mTexChannels = new TextureHandle[(int)BTerrainTexturing.eTextureChannels.cSplatChannelCount];

         if (!File.Exists(mFilename))
         {
            mFilename = EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName;
            mTexChannels[0] = BRenderDevice.getTextureManager().getTexture(mFilename);// = TextureLoader.FromFile(BRenderDevice.getDevice(),mFilename);
            mTexChannels[1] = BRenderDevice.getTextureManager().getTexture(mFilename);// = TextureLoader.FromFile(BRenderDevice.getDevice(), mFilename);

            return;
         }


         mTexChannels[0] = BRenderDevice.getTextureManager().getTexture(mFilename, TerrainGlobals.getTexturing().activeTextureReloaded);//         mTexChannels[0] = TextureLoader.FromFile(BRenderDevice.getDevice(), mFilename);

         String ext = Path.GetExtension(mFilename);
         string tName = mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_nm" + ext;
         mTexChannels[1] = BRenderDevice.getTextureManager().getTexture(tName, TerrainGlobals.getTexturing().activeTextureReloaded);// = TextureLoader.FromFile(BRenderDevice.getDevice(), tName);


         //gather our information data for the editor
         mMatHasEmissive = File.Exists(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_em" + ext);
         mMatHasEnvMask = File.Exists(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_rm" + ext);
         mMatHasSpecular = File.Exists(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_sp" + ext);
         mMatHasMask = File.Exists(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_mk" + ".bmp");

         //calculate our total memory footprint for this texture
         m360MemoryFootprint = 0;
         m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_df.ddx");
         m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_nm.ddx");
         m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_em.ddx");
         m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_rm.ddx");
         m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_sp.ddx");

      }
      

      public string getMaskFileName()
      {
         return mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_mk" + ".bmp";
      }


      public TextureHandle[] mTexChannels;
      public string mFilename;
      public Bitmap mThumbnail;

      //material properties
      public int mUScale=1;
      public int mVScale=1;
      public int mBlendOp=0;
      public float mSpecExponent = 1.0f;
      public float mFresnelPower = 3.5f;
      public float mFresnelScale = 20.0f;
      public float mFresnelBias = 0.0f;
      public float mFresnelRefractPercent = 0.75f;

      //info data used to display on screen
      public bool mMatHasSpecular = false;
      public bool mMatHasEmissive = false;
      public bool mMatHasEnvMask = false;
      public bool mMatHasMask = false;

      public int m360MemoryFootprint = 0;
   }

   //------------------------------------------------------------
   //this is a wrapper for our texturing and LOD evaluation, used as a packet for BTerrainRender
   public class BTerrainTexturingDataHandle
   {
      public BTerrainTexturingDataHandle()
      {
         /*
         mIndexesTexture = new Texture(BRenderDevice.getDevice(), (int)BTerrainTexturing.getAlphaTextureWidth(), (int)BTerrainTexturing.getAlphaTextureHeight(), 1, 0, Format.A4R4G4B4, Pool.Managed);
         mAlphasTexture = new Texture(BRenderDevice.getDevice(), (int)BTerrainTexturing.getAlphaTextureWidth(), (int)BTerrainTexturing.getAlphaTextureHeight(), 1, 0, Format.A4R4G4B4, Pool.Managed);
         */
      }
      ~BTerrainTexturingDataHandle()
      {
         Dispose();
      }
      public BTerrainTexturingDataHandle copy()
      {
         BTerrainTexturingDataHandle newCopy = new BTerrainTexturingDataHandle();

         newCopy.Dispose();
         //Texture
         /*
         newCopy.mAlphasTexture = TextureLoader.FromStream(BRenderDevice.getDevice(), TextureLoader.SaveToStream(ImageFileFormat.Bmp, mAlphasTexture));
         newCopy.mIndexesTexture = TextureLoader.FromStream(BRenderDevice.getDevice(), TextureLoader.SaveToStream(ImageFileFormat.Bmp, mIndexesTexture));
         */
         
         return newCopy;
      }

      public void Dispose()
      {
     
         /*
         if (mIndexesTexture != null)
         {
            if (mIndexesTexture.Disposed == false)
            {
               mIndexesTexture.Dispose();
               mIndexesTexture = null;
            }
         }
         if (mAlphasTexture != null)
         {
            if (mAlphasTexture.Disposed == false)
            {
               mAlphasTexture.Dispose();
               mAlphasTexture = null;
            }
         }
         */

         if(mCompositeTexture!=null)
         {
            mCompositeTexture.destroy();
            mCompositeTexture = null;
         }
      }

      public float evaluateAPC()
      {
         int c=0;
         for (int i = 0; i < 32; i++)
            if ((mAgePrediction & (1 << i))!=0)
               c++;
         return (float)c / 32.0f;
      }

      public void evaluateLOD()//CLM this is only called on visible nodes
      {
         mAgePrediction = mAgePrediction | 0x00000001;
      }

      public void evaluateLOD(Vector3 pt, Vector3 eye, BTerrainLayerContainer layers,BTerrainQuadNode ownerQN)//CLM this is only called on visible nodes
      {
         mAgePrediction = mAgePrediction | 0x00000001;

         int lod = TerrainGlobals.getTexturing().getTextureLODLevel(pt,eye);
         if (lod != mTextureLODLevel)// || mCompositeTexture==null || mCompositeTexture.mState==eCompositeTextureState.cCS_Free)
         {
            mTextureLODLevel = lod;
            if (mCompositeTexture!=null)
            {
               mCompositeTexture.free();
               mCompositeTexture = null;
            }
            /*
            mCompositeTexture = TerrainGlobals.getTexturing().getCompositeTexture((int)(BTerrainTexturing.getTextureWidth() >> mTextureLODLevel), (int)(BTerrainTexturing.getTextureHeight() >> mTextureLODLevel));
            if (mCompositeTexture != null)
            {
               mCompositeTexture.mOwnerNode = ownerQN;
               layers.convertToTexturingDataHandle(ref mCompositeTexture);
            }*/
         }
      }
      public void updateLOD()
      {
         mAgePrediction = mAgePrediction << 1;
         if (mCompositeTexture!=null)
         {

            if (0==(mAgePrediction & cVisibleFrameHold))
            {
               mCompositeTexture.free();
               mCompositeTexture = null;
            }
         }    
      }
      public void free()
      {
         if (mCompositeTexture!=null)
         {
            mCompositeTexture.free();
            mCompositeTexture = null;
         }
      }

/*
      public Texture mIndexesTexture = null;
      public Texture mAlphasTexture = null;
*/
      public BTerrainCompositeTexture mCompositeTexture = null;
      public int mTextureLODLevel = 0;
      
      static public int cVisibleFrameHold = 0x00000FFF;
      public uint mAgePrediction = 0;
   };
   //------------------------------------------------------------
   public partial class BTerrainTexturing
   {
      public enum eTextureChannels
      {
         cAlbedo = 0,
         cNormal,
         cOpacity,
         cSplatChannelCount = 2,
         cDecalChannelCount = 3,

      };


      static public int widthToLOD(int width)
      {
         int lod = 0;
         switch (width)
         {
            case 512: lod = 0; break;
            case 256: lod = 1; break;
            case 128: lod = 2; break;
            case 64: lod = 3; break;
            case 32: lod = 4; break;
            case 16: lod = 5; break;
         }
         return lod;
      }
      static public int LODToWidth(int lod)
      {
         int width = 0;
         switch (lod)
         {
            case 0: width = 512; break;
            case 1: width = 256; break;
            case 2: width = 128; break;
            case 3: width = 64; break;
            case 4: width = 32; break;
            case 5: width = 16; break;
         }
         return width;
      }

      public BTerrainTexturing()
      {

      }
      ~BTerrainTexturing()
      {
         destroyCaches();
         destroy();
      }

      

      public   void           init()
      {
         initCaches();

         initComposite(); 
      }
      public   void           destroy()
      {
         destroyActiveTextures();
         destroyActiveDecals();
         destroyContainers();
         //destroyCaches();//CLM why is this commented out?

         if (mCompositeShader != null)
         {
            mCompositeShader.Dispose();
            mCompositeShader = null;
         }

         if (mDepthStenSurf!=null)
         {
            mDepthStenSurf.Dispose();
            mDepthStenSurf = null;
         }
         if (mTempAlphaTextures != null)
         {
            for (int i = 0; i < mTempAlphaTextures.Count; i++)
            {
               mTempAlphaTextures[i].Dispose();
               mTempAlphaTextures[i] = null;
            }
            mTempAlphaTextures.Clear();
         }
         if(mCompositeVertexBuffer!=null)
         {
            mCompositeVertexBuffer.Dispose();
            mCompositeVertexBuffer=null;
         }
         if(mCompositeIndexBuffer!=null)
         {
            mCompositeIndexBuffer.Dispose();
            mCompositeIndexBuffer=null;
         }
         if(mVertexDecl!=null)
         {
            mVertexDecl.Dispose();
            mVertexDecl=null;
         }

      }

      public void d3dDeviceLost()
      {
         if (savedRenderTarget != null)
         {
            savedRenderTarget.Dispose();
            savedRenderTarget = null;
         }
         if (savedDepthSurf != null)
         {

            savedDepthSurf.Dispose();
            savedDepthSurf = null;
         }


         destroyCaches();
         
      }
      public void d3dDeviceRestored()
      {
         initCaches();
      }

      //statics
      public static uint getTextureWidth()       { return cVisualTextureImageWidth; }    //THIS MUST MATCH THE GAME
      public static uint getTextureHeight()      { return cVisualTextureImageHeight; }    //THIS MUST MATCH THE GAME
      public static uint getAlphaTextureWidth()  { return cAlphaTextureWidth; }    //THIS MUST MATCH THE GAME
      public static uint getAlphaTextureHeight() { return cAlphaTextureHeight; }    //THIS MUST MATCH THE GAME


      //generally used
      public void reloadCachedVisuals()
      {
         freeAllCaches();
         TerrainGlobals.getTerrain().getQuadNodeRoot().freeCacheHandles();
      }

      //save / load / export
      public void preExportTo360()
      {
         //flattenLayers();
         removeUnusedActiveTextures();
         removeUnusedActiveDecals();
         ensureCleanLayers();
      }
      public void preSaveToTED()
      {
         removeUnusedActiveTextures();
         removeUnusedActiveDecals();
         ensureCleanLayers();
      }


      //active texture list management
      List<BTerrainActiveTextureContainer> mActiveTextures = new List<BTerrainActiveTextureContainer>();
      public int getActiveTextureCount()
      {
         return mActiveTextures.Count;
      }
      public int addActiveTexture(String filename)
      {
         for (int i = 0; i < mActiveTextures.Count; i++)
            if (mActiveTextures[i].mFilename == filename)
               return i;

         BTerrainActiveTextureContainer atc = new BTerrainActiveTextureContainer();
         atc.mFilename = filename;
         atc.loadTextures();
         mActiveTextures.Add(atc);

         BRenderDevice.getTextureManager().addWatchedTexture(filename, activeTextureReloaded);
         return mActiveTextures.Count - 1;
      }
      public void removeActiveTexture(int activeTextureIndex)
      {
         if (activeTextureIndex < 0 || activeTextureIndex >= mActiveTextures.Count)
            return;

         for (int k = 0; k < mContainers.Count; k++)
         {
            mContainers[k].removeLayersWithTexID(activeTextureIndex,BTerrainTexturingLayer.eLayerType.cLayer_Splat, false);
            mContainers[k].activeIndexRemovedCascade(activeTextureIndex, BTerrainTexturingLayer.eLayerType.cLayer_Splat);
            mContainers[k].removeLayersWithWrongID();
            mContainers[k].removeBlankLayers();
         }

         mActiveTextures.RemoveAt(activeTextureIndex);
         reloadCachedVisuals();
      }
      public void removeUnusedActiveTextures()
      {
         
         for (int i = 0; i < mActiveTextures.Count; i++)
         {
            if (!activeTextureUsed(i))
            {
               removeActiveTexture(i);
               SimEditor.SimTerrainType.removeActiveSetDef(i);
               i--;
            }
         }
          
      }
      public BTerrainActiveTextureContainer getActiveTexture(int activeTextureIndex)
      {
         if (activeTextureIndex < 0 || activeTextureIndex >= mActiveTextures.Count)
            return null;

         return mActiveTextures[activeTextureIndex];
      }
      public void destroyActiveTextures()
      {
         for (int i = 0; i < mActiveTextures.Count; i++)
            mActiveTextures[i].destroy();
         mActiveTextures.Clear();
      }
      public void reloadActiveTextures(bool force)
      {
         freeAllCaches();
         for (int i = 0; i < mActiveTextures.Count; i++)
         {
            mActiveTextures[i].destroyDeviceData();// destroy();
            mActiveTextures[i].loadTextures();
         }
        // mActiveTextures.Clear();
        
      }
      public void createTexturesFromSimDefs()
      {
         mActiveTextures.Clear();

         List<TerrainTextureDef> activeTexDefs = SimTerrainType.getDefsOfActiveSet();

         for (int i = 0; i < activeTexDefs.Count; i++)
         {
            if (BRenderDevice.getDevice() != null)
            {
               string fullFileName = SimTerrainType.getWorkingTexName(activeTexDefs[i]);
               addActiveTexture(fullFileName);
            }
         }
      }
      public void activeTextureReloaded(string filename)
      {
         reloadCachedVisuals();
      }
      public void swapActiveTextures(int orig, int next)
      {
         if (orig > next)
         {
            int k = orig;
            orig = next;
            next = k;
         }
         mActiveTextures.Insert(orig, mActiveTextures[next]);
         mActiveTextures.Insert(next+1, mActiveTextures[orig+1]);
         mActiveTextures.RemoveAt(orig + 1);
         mActiveTextures.RemoveAt(next + 1);
      }
      public void replaceActiveTexture(int orig, int next)
      {
         replaceLayerIDs(orig, next);
         removeActiveTexture(orig);
         normalizeLayers();

         //Masking.clearSelectionMask();
         //TerrainGlobals.getEditor().TextureToMask(orig);
         ////swapActiveTextures(orig, next);
         //int oldTex = TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex;
         //TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex = next;
         //TerrainGlobals.getEditor().applySelectedTextureToMask();
         //removeActiveTexture(orig);
         //Masking.clearSelectionMask();
         //TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex = 0;
         
        // replaceLayerIDs(next, orig);
        // swapActiveTextures(orig, next);
       //  removeActiveTexture(next);

       //  mActiveTextures.Insert(orig, mActiveTextures[next]);
       //  mActiveTextures.RemoveAt(orig + 1);
         //mActiveTextures.RemoveAt(next);
      }

      

      //containers cache -- CLM we keep our containers for cascade operations
     List<BTerrainLayerContainer> mContainers = new List<BTerrainLayerContainer>();   
      public BTerrainLayerContainer getNewContainer()
      {
         mContainers.Add(new BTerrainLayerContainer());
         mContainers[mContainers.Count - 1].makeBaseLayer(0);
         return mContainers[mContainers.Count - 1];
      }
      public void destroyContainers()
      {
         for (int i = 0; i < mContainers.Count; i++)
            mContainers[i].destroy();
         mContainers.Clear();
      }
      public BTerrainLayerContainer getContainer(int containerIndex)
      {
         if (containerIndex < 0 || containerIndex >= mContainers.Count)
            return null;
         return mContainers[containerIndex];
      }
      public void ensureCleanLayers()
      {
         
         //for (int i = 0; i < mContainers.Count; i++)
         //{
         //   mContainers[i].removeBlankLayers();
         //   mContainers[i].removeLayersWithWrongID();
         //}
      }
      public void removeBlankTextures()
      {
         for (int i = 0; i < mContainers.Count; i++)
            mContainers[i].removeBlankLayers();
         reloadCachedVisuals();
      }
      public bool activeTextureUsed(int activeTextureIndex)
      {
         for (int i = 0; i < mContainers.Count; i++)
         {
            if (mContainers[i].containsID(activeTextureIndex, BTerrainTexturingLayer.eLayerType.cLayer_Splat))
               return true;
         }
         return false;
      }
      public bool activeDecalInstanceUsed(int activeDecalInstanceIndex)
      {
         for (int i = 0; i < mContainers.Count; i++)
         {
            if (mContainers[i].containsID(activeDecalInstanceIndex, BTerrainTexturingLayer.eLayerType.cLayer_Decal))
               return true;
         }
         return false;
      }
      public void replaceLayerIDs(int orig, int next)
      {
         for (int i = 0; i < mContainers.Count; i++)
         {
          //  int oldIDX = mContainers[i].giveLayerIndex(orig, BTerrainTexturingLayer.eLayerType.cLayer_Splat);
            mContainers[i].replaceIDWithID(orig, next, false);
          //  mContainers[i].removeLayer(oldIDX);
            mContainers[i].removeBlankLayers();
         }
         reloadCachedVisuals();
      }
      public void swapLayerIDs(int orig, int next)
      {
         int markedValue = 12345678;
         for (int i = 0; i < mContainers.Count; i++)
            mContainers[i].replaceIDWithID(orig, markedValue, false);

         for (int i = 0; i < mContainers.Count; i++)
            mContainers[i].replaceIDWithID(next, orig, false);

         for (int i = 0; i < mContainers.Count; i++)
            mContainers[i].replaceIDWithID(markedValue, next, false);

         ensureCleanLayers();



         reloadCachedVisuals();
      }
      public void normalizeLayers()
      {
         for (int i = 0; i < mContainers.Count; i++)
            mContainers[i].ensureProperLayerOrdering();

         reloadCachedVisuals();
      }

      //unique texture caching
      List<BTerrainCompositeCache> mCaches = null;
      public void initCaches()
      {
         if (mCaches!=null)
            return;

         mCaches = new List<BTerrainCompositeCache>();
         int[] startingReserve = new int[] { 8, 16, 32, 64, 256 , 2048 };
         for (int i = 0; i < cMaxNumLevels; i++)
         {
            BTerrainCompositeCache cc = new BTerrainCompositeCache();
            cc.create((int)(cVisualTextureImageWidth >> i), (int)(cVisualTextureImageHeight >> i), startingReserve[i]);
            mCaches.Add(cc);
         }

      }
      public void destroyCaches()
      {
         if (mCaches!=null)
         {
            freeAllCaches();
            for (int i = 0; i < mCaches.Count; i++)
               mCaches[i].destroy();
            mCaches = null;
         }
      }
      public void freeAllCaches()
      {
         if (mCaches != null)
         {
            for (int i = 0; i < mCaches.Count; i++)
               mCaches[i].free();
         }
      }
      public BTerrainCompositeTexture getCompositeTexture(int width, int height)
      {
         BTerrainCompositeTexture tex = null;
         for (int i = 0; i < cMaxNumLevels; i++)
         {
            if (mCaches[i].mWidth == width && mCaches[i].mHeight == height)
            {
               return mCaches[i].getCompositeTexture();

            }
         }
         //we didn't find a free texture, try for a used one

         return null;
         
      }
      public int getTextureLODLevel(Vector3 pt, Vector3 Eye)
      {
         Vector3 d = pt - Eye;
         float dist = d.Length();

         float[] distances = new float[] { 200.0f, 250.0f, 275.0f, 300.0f, 325.0f, 350.0f };

         for (int i = 0; i < cMaxNumLevels; i++)
         {
            if(dist < distances[i])
               return i;
         }
         return cMaxNumLevels-1;
         
      }
      public void preConvertSetup()
      {
         if (savedRenderTarget != null) 
         {
            savedRenderTarget.Dispose();
            savedRenderTarget = null;
         }
         if (savedDepthSurf != null)
         {

            savedDepthSurf.Dispose();
            savedDepthSurf = null;
         }

         BRenderDevice.getDevice().RenderState.CullMode = Cull.None;
         BRenderDevice.getDevice().RenderState.ZBufferWriteEnable = false;
         BRenderDevice.getDevice().RenderState.ZBufferEnable = false;
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.PosW_uv0_uv1.vertDecl;
         BRenderDevice.getDevice().RenderState.AlphaBlendEnable = true;
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Microsoft.DirectX.Direct3D.Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Microsoft.DirectX.Direct3D.Blend.InvSourceAlpha);

         savedViewport = BRenderDevice.getDevice().Viewport;
         savedRenderTarget = BRenderDevice.getDevice().GetRenderTarget(0);
         savedDepthSurf = BRenderDevice.getDevice().DepthStencilSurface;
         BRenderDevice.getDevice().DepthStencilSurface = mDepthStenSurf;

         BRenderDevice.getDevice().SetStreamSource(0, mCompositeVertexBuffer, 0);
         BRenderDevice.getDevice().Indices = mCompositeIndexBuffer;
      }
      public void postConvertSetup()
      {
         BRenderDevice.getDevice().RenderState.ZBufferEnable = true;
         BRenderDevice.getDevice().RenderState.AlphaBlendEnable = false;
         BRenderDevice.getDevice().Viewport = savedViewport;
         BRenderDevice.getDevice().DepthStencilSurface = savedDepthSurf;
         BRenderDevice.getDevice().SetRenderTarget(0, savedRenderTarget);
       //  BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().RenderState.ZBufferWriteEnable = true;
      }



      static uint cAlphaTextureWidth = 64;
      static uint cAlphaTextureHeight = 64;

      static uint cVisualTextureImageWidth = 512;
      static uint cVisualTextureImageHeight = 512;
      public static int cMaxDecalsPerChunk = 8;

      public static int cMaxNumMips = 1;
      public static int cMaxNumLevels = 6;
      public static int cMaxNumBlends = 16;
   }
   //------------------------------------------------------------

   #endregion
}




//------------------------------------------------------------
