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
   //this is used to keep store of all textures in active set
   public class BTerrainActiveDecalContainer
   {
      public void destroy()
      {
         if (mTexChannels != null)
         {
            for (int i = 0; i < (int)BTerrainTexturing.eTextureChannels.cDecalChannelCount; i++)
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
         for (int i = 0; i < (int)BTerrainTexturing.eTextureChannels.cDecalChannelCount; i++)
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
         if (mTexChannels == null)
            mTexChannels = new TextureHandle[(int)BTerrainTexturing.eTextureChannels.cDecalChannelCount + 1];

         if (!File.Exists(mFilename))
         {
            mFilename = EditorCore.CoreGlobals.getWorkPaths().mBlankTextureName;
            mTexChannels[(int)BTerrainTexturing.eTextureChannels.cAlbedo] = BRenderDevice.getTextureManager().getTexture(mFilename);// = TextureLoader.FromFile(BRenderDevice.getDevice(),mFilename);
            mTexChannels[(int)BTerrainTexturing.eTextureChannels.cNormal] = BRenderDevice.getTextureManager().getTexture(mFilename);// = TextureLoader.FromFile(BRenderDevice.getDevice(), mFilename);
            mTexChannels[(int)BTerrainTexturing.eTextureChannels.cOpacity] = BRenderDevice.getTextureManager().getTexture(mFilename);// = TextureLoader.FromFile(BRenderDevice.getDevice(), mFilename);

            return;
         }


         mTexChannels[(int)BTerrainTexturing.eTextureChannels.cAlbedo] = BRenderDevice.getTextureManager().getTexture(mFilename, TerrainGlobals.getTexturing().activeDecalReloaded);//         mTexChannels[0] = TextureLoader.FromFile(BRenderDevice.getDevice(), mFilename);

         String ext = Path.GetExtension(mFilename);
         string tName = mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_nm" + ext;
         mTexChannels[(int)BTerrainTexturing.eTextureChannels.cNormal] = BRenderDevice.getTextureManager().getTexture(tName, TerrainGlobals.getTexturing().activeDecalReloaded);// = TextureLoader.FromFile(BRenderDevice.getDevice(), tName);

         tName = mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_op" + ext;
         mTexChannels[(int)BTerrainTexturing.eTextureChannels.cOpacity] = BRenderDevice.getTextureManager().getTexture(tName, TerrainGlobals.getTexturing().activeDecalReloaded);// = TextureLoader.FromFile(BRenderDevice.getDevice(), tName);

         SurfaceDescription sd = mTexChannels[(int)BTerrainTexturing.eTextureChannels.cAlbedo].mTexture.GetLevelDescription(0);

         mWidth = sd.Width;
         mHeight = sd.Height;

         //calculate our total memory footprint for this texture
         m360MemoryFootprint = 0;
         //m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_df.ddx");
         //m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_nm.ddx");
         //m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_em.ddx");
         //m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_rm.ddx");
         //m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_sp.ddx");
         //m360MemoryFootprint += DDXBridge.give360TextureMemFootprint(mFilename.Substring(0, mFilename.LastIndexOf("_df")) + "_op.ddx");

      }


      public TextureHandle[] mTexChannels;
      public string mFilename;
      public int mWidth;
      public int mHeight;
      public int m360MemoryFootprint = 0;
   }
   public class BTerrainDecalInstance
   {
      public int mActiveDecalIndex;
      public float mRotation;
      public float mUScale;
      public float mVScale;
      public Vector2 mTileCenter;
      public Vector2 mTileBoundsMin;
      public Vector2 mTileBoundsMax;
      public bool mIsSelected;         //editor can select a decal instance

      public void computeBounds()
      {
         float vertsToHighResPixelSpaceRatio = BTerrainTexturing.getTextureWidth() / BTerrainQuadNode.cMaxWidth;

         BTerrainActiveDecalContainer dcl = TerrainGlobals.getTexturing().getActiveDecal(mActiveDecalIndex);
         int h = (int)(dcl.mWidth * (1.0f / mUScale));
         int w = (int)(dcl.mHeight * (1.0f / mVScale));

         //rotate our bounds 
         float angleRad = mRotation;// -rotAngle * Math.PI / 180;
         float angleCos = (float)Math.Cos(angleRad);
         float angleSin = (float)Math.Sin(angleRad);

         float halfWidth = w >> 1;
         float halfHeight = h >> 1;

         float halfNewWidth, halfNewHeight;
         int newWidth, newHeight;


         // rotate corners
         float cx1 = halfWidth * angleCos;
         float cy1 = halfWidth * angleSin;

         float cx2 = halfWidth * angleCos - halfHeight * angleSin;
         float cy2 = halfWidth * angleSin + halfHeight * angleCos;

         float cx3 = -halfHeight * angleSin;
         float cy3 = halfHeight * angleCos;

         float cx4 = 0;
         float cy4 = 0;

         halfNewWidth = Math.Max(Math.Max(cx1, cx2), Math.Max(cx3, cx4)) - Math.Min(Math.Min(cx1, cx2), Math.Min(cx3, cx4));
         halfNewHeight = Math.Max(Math.Max(cy1, cy2), Math.Max(cy3, cy4)) - Math.Min(Math.Min(cy1, cy2), Math.Min(cy3, cy4));

         newWidth = (int)(halfNewWidth * 2 + 0.5);
         newHeight = (int)(halfNewHeight * 2 + 0.5);


         w = newWidth >> 1;
         h = newHeight >> 1;


         mTileBoundsMin.X = (int)((mTileCenter.X - (w)) / vertsToHighResPixelSpaceRatio);
         mTileBoundsMax.X = (int)((mTileCenter.X + (w)) / vertsToHighResPixelSpaceRatio);
         mTileBoundsMin.Y = (int)((mTileCenter.Y - (h)) / vertsToHighResPixelSpaceRatio);
         mTileBoundsMax.Y = (int)((mTileCenter.Y + (h)) / vertsToHighResPixelSpaceRatio);



      }
   }

   public partial class BTerrainTexturing
   {
      //active decal list management
      List<BTerrainActiveDecalContainer> mActiveDecals = new List<BTerrainActiveDecalContainer>();
      public int getActiveDecalCount()
      {
         return mActiveDecals.Count;
      }
      public BTerrainActiveDecalContainer getActiveDecal(int activeDecalIndex)
      {
         if (activeDecalIndex < 0 || activeDecalIndex >= mActiveDecals.Count)
            return null;

         return mActiveDecals[activeDecalIndex];
      }
      public int addActiveDecal(String filename)
      {
         for (int i = 0; i < mActiveDecals.Count; i++)
            if (mActiveDecals[i].mFilename == filename)
               return i;

         BTerrainActiveDecalContainer atc = new BTerrainActiveDecalContainer();
         atc.mFilename = filename;
         atc.loadTextures();
         mActiveDecals.Add(atc);

         BRenderDevice.getTextureManager().addWatchedTexture(filename, activeTextureReloaded);
         return mActiveDecals.Count - 1;
      }
      public int getActiveDecalIndex(String filename)
      {
         for (int i = 0; i < mActiveDecals.Count; i++)
            if (mActiveDecals[i].mFilename == filename)
               return i;
         return -1;
      }
      public void removeActiveDecal(int activeDecalIndex)
      {
         if (activeDecalIndex < 0 || activeDecalIndex >= mActiveDecals.Count)
            return;


         //remove our instances
         for (int i = 0; i < mDecalInstances.Count; i++)
         {
            if (mDecalInstances[i].mActiveDecalIndex == activeDecalIndex)
            {
               removeActiveDecalInstance(i);
               //mDecalInstances.RemoveAt(i);
               i--;
            }
            else if (mDecalInstances[i].mActiveDecalIndex > activeDecalIndex)
            {
               mDecalInstances[i].mActiveDecalIndex--;
            }
         }

         mActiveDecals.RemoveAt(activeDecalIndex);

         reloadCachedVisuals();
      }
      public void removeUnusedActiveDecals()
      {
         removeUnusedActiveDecalInstances();

         for (int x = 0; x < mActiveDecals.Count; x++)
         {
            bool found = false;
            for (int i = 0; i < mDecalInstances.Count; i++)
            {
               if (mDecalInstances[i].mActiveDecalIndex == x)
               {
                  found = true;
                  break;
               }
            }
            if (!found)
            {
               removeActiveDecal(x);
               x--;
            }
         }
      }
      public void destroyActiveDecals()
      {
         for (int i = 0; i < mActiveDecals.Count; i++)
            mActiveDecals[i].destroy();
         mActiveDecals.Clear();

         mDecalInstances.Clear();
      }
      public void reloadActiveDecals(bool force)
      {
         freeAllCaches();
         for (int i = 0; i < mActiveDecals.Count; i++)
         {
            mActiveDecals[i].destroyDeviceData();// destroy();
            mActiveDecals[i].loadTextures();
         }
      }
      public void activeDecalReloaded(string filename)
      {
         reloadCachedVisuals();
      }
      public void swapActiveDecals(int orig, int next)
      {
         if (orig > next)
         {
            int k = orig;
            orig = next;
            next = k;
         }
         mActiveDecals.Insert(orig, mActiveDecals[next]);
         mActiveDecals.Insert(next + 1, mActiveDecals[orig + 1]);
         mActiveDecals.RemoveAt(orig + 1);
         mActiveDecals.RemoveAt(next + 1);
      }
      public void replaceActiveDecal(int orig, int next)
      {
         replaceLayerIDs(next, orig);
         swapActiveDecals(orig, next);
         removeActiveDecal(next);
      }
      public void selectActiveDecal(int activeDecalIndex)
      {
         if (activeDecalIndex < 0 || activeDecalIndex >= mActiveDecals.Count)
            return;

         for (int i = 0; i < mDecalInstances.Count; i++)
         {
            if (mDecalInstances[i].mActiveDecalIndex == activeDecalIndex)
               selectActiveDecalInstance(i);
         }
      }
      public int getActiveDecalNumInstances(int activeDecalIndex)
      {
         int instanceCount = 0;
         for (int i = 0; i < mDecalInstances.Count; i++)
         {
            if (mDecalInstances[i].mActiveDecalIndex == activeDecalIndex)
               instanceCount++;
         }
         return instanceCount;
      }


      List<BTerrainDecalInstance> mDecalInstances = new List<BTerrainDecalInstance>();
      public int addActiveDecalInstance(int activeDecalIndex, int xPos, int zPos, float uScale, float vScale, float rotation, int minXTile, int maxXTile, int minYTile, int maxYTile)
      {
         BTerrainDecalInstance di = new BTerrainDecalInstance();
         di.mActiveDecalIndex = activeDecalIndex;
         di.mRotation = rotation;
         di.mUScale = uScale;
         di.mVScale = vScale;
         di.mTileCenter = new Vector2(xPos, zPos);

         di.mTileBoundsMin.X = minXTile;
         di.mTileBoundsMin.Y = minYTile;
         di.mTileBoundsMax.X = maxXTile;
         di.mTileBoundsMax.Y = maxYTile;
         di.mIsSelected = false;

         mDecalInstances.Add(di);

         return mDecalInstances.Count - 1;
      }
      public int getActiveDecalInstancesCount()
      {
         return mDecalInstances.Count;
      }

      public void removeActiveDecalInstance(int activeDecalInstanceIndex)
      {
         if (activeDecalInstanceIndex < 0 || activeDecalInstanceIndex >= mDecalInstances.Count)
            return;

         for (int k = 0; k < mContainers.Count; k++)
         {
            mContainers[k].removeLayersWithTexID(activeDecalInstanceIndex, BTerrainTexturingLayer.eLayerType.cLayer_Decal, false);
            mContainers[k].activeIndexRemovedCascade(activeDecalInstanceIndex, BTerrainTexturingLayer.eLayerType.cLayer_Decal);
            mContainers[k].removeLayersWithWrongID();
            mContainers[k].removeBlankLayers();
         }

         mDecalInstances.RemoveAt(activeDecalInstanceIndex);
         reloadCachedVisuals();
      }
      public BTerrainDecalInstance getActiveDecalInstance(int activeDecalInstanceIndex)
      {
         if (activeDecalInstanceIndex < 0 || activeDecalInstanceIndex >= mDecalInstances.Count)
            return null;

         return mDecalInstances[activeDecalInstanceIndex];
      }
      public void removeUnusedActiveDecalInstances()
      {
         for (int i = 0; i < mDecalInstances.Count; i++)
         {
            if (!activeDecalInstanceUsed(i))
            {
               removeActiveDecalInstance(i);
               i--;
            }
         }
      }
      public int getSelectedDecalInstanceCount()
      {
         int c = 0;
         for (int i = 0; i < mDecalInstances.Count; i++)
            if (mDecalInstances[i].mIsSelected)
               c++;

         return c;
      }
      public void selectActiveDecalInstance(int activeDecalInstanceIndex)
      {
         if (activeDecalInstanceIndex < 0 || activeDecalInstanceIndex >= mDecalInstances.Count)
            return;
         mDecalInstances[activeDecalInstanceIndex].mIsSelected = true;

         List<BTerrainQuadNode> nodes = TerrainGlobals.getTexturing().giveNodesIntersectingDecalInstance(activeDecalInstanceIndex);
         for (int i = 0; i < nodes.Count; i++)
         {
            //nodes[i].getTextureData().free();
            for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
            {
               nodes[i].getTextureData(lod).free();
            }
         }
         nodes.Clear();
         nodes = null;
      }
      public void unselectActiveDecalInstance(int activeDecalInstanceIndex)
      {
         if (activeDecalInstanceIndex < 0 || activeDecalInstanceIndex >= mDecalInstances.Count)
            return;
         mDecalInstances[activeDecalInstanceIndex].mIsSelected = false;

         List<BTerrainQuadNode> nodes = TerrainGlobals.getTexturing().giveNodesIntersectingDecalInstance(activeDecalInstanceIndex);
         for (int i = 0; i < nodes.Count; i++)
         {
            //nodes[i].getTextureData().free();
            for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
            {
               nodes[i].getTextureData(lod).free();
            }
         }
         nodes.Clear();
         nodes = null;
      }
      public void unselectAllDecalInstances()
      {
         for (int i = 0; i < mDecalInstances.Count; i++)
         {
            if (mDecalInstances[i].mIsSelected)
            {
               unselectActiveDecalInstance(i);
            }
         }
      }
      public void removeSelectedDecalInstances()
      {
         for (int i = 0; i < mDecalInstances.Count; i++)
         {
            if (mDecalInstances[i].mIsSelected)
            {
               removeActiveDecalInstance(i);
               i--;
            }
         }
         unselectAllDecalInstances();
      }
      public List<BTerrainQuadNode> giveNodesIntersectingDecalInstance(int activeDecalInstanceIndex)
      {
         if (activeDecalInstanceIndex < 0 || activeDecalInstanceIndex >= mDecalInstances.Count)
            return null;

         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, (int)mDecalInstances[activeDecalInstanceIndex].mTileBoundsMin.X,
                                                                                 (int)mDecalInstances[activeDecalInstanceIndex].mTileBoundsMax.X,
                                                                                 (int)mDecalInstances[activeDecalInstanceIndex].mTileBoundsMin.Y,
                                                                                 (int)mDecalInstances[activeDecalInstanceIndex].mTileBoundsMax.Y);

         return nodes;
      }
      //CLM THIS MATH IS VERY TRICKY!! PLEASE TALK TO ME BEFORE SCREWING WITH IT!!!!
      public void rotateSelectedDecals(float rotAmt)
      {
         for (int i = 0; i < mDecalInstances.Count; i++)
         {
            if (mDecalInstances[i].mIsSelected)
            {
               mDecalInstances[i].mRotation += rotAmt;

               recomputeDecalInstanceBounds(i, true);
            }
         }
      }
      public void resizeSelectedDecals(float xScaleAmt, float yScaleAmt)
      {
         float vertsToHighResPixelSpaceRatio = BTerrainTexturing.getTextureWidth() / BTerrainQuadNode.cMaxWidth;

         for (int i = 0; i < mDecalInstances.Count; i++)
         {
            if (mDecalInstances[i].mIsSelected)
            {
               BTerrainActiveDecalContainer dcl = mActiveDecals[mDecalInstances[i].mActiveDecalIndex];

               mDecalInstances[i].mUScale += xScaleAmt;
               mDecalInstances[i].mVScale += yScaleAmt;


               recomputeDecalInstanceBounds(i, true);
            }
         }
      }
      public void moveSelectdDecalsFromEditorInput()
      {
         if (mDecalInstances.Count == 0)
            return;

         int xCenterGlobal = 0;
         int zCenterGlobal = 0;

         //CLM THIS MATH IS VERY TRICKY!! PLEASE TALK TO ME BEFORE SCREWING WITH IT!!!!
         //CLM THIS MATH IS VERY TRICKY!! PLEASE TALK TO ME BEFORE SCREWING WITH IT!!!!

         float vertsToHighResPixelSpaceRatio = BTerrainTexturing.getTextureWidth() / BTerrainQuadNode.cMaxWidth;

         int mVisTileIntetersectionX = TerrainGlobals.getEditor().mVisTileIntetersectionX;
         int mVisTileIntetersectionZ = TerrainGlobals.getEditor().mVisTileIntetersectionZ;

         Vector3 intpt = TerrainGlobals.getEditor().mBrushIntersectionPoint;

         for (int i = 0; i < mDecalInstances.Count; i++)
         {
            if (mDecalInstances[i].mIsSelected)
            {
               int myTileX = mVisTileIntetersectionX;// (int)((mDecalInstances[i].mTileCenter.X / vertsToHighResPixelSpaceRatio) + (diffX));
               int myTileZ = mVisTileIntetersectionZ;// (int)((mDecalInstances[i].mTileCenter.Y / vertsToHighResPixelSpaceRatio) + (diffZ));

               BTerrainActiveDecalContainer dcl = mActiveDecals[mDecalInstances[i].mActiveDecalIndex];

               Vector3 a = TerrainGlobals.getTerrain().getPostDeformPos(myTileX, myTileZ);
               Vector3 b = TerrainGlobals.getTerrain().getPostDeformPos(myTileX + 1, myTileZ);
               Vector3 c = TerrainGlobals.getTerrain().getPostDeformPos(myTileX, myTileZ + 1);
               Vector3 d = TerrainGlobals.getTerrain().getPostDeformPos(myTileX + 1, myTileZ + 1);

               //find the max def for this tile
               float x1 = (float)Math.Abs(a.X - b.X);
               float x2 = (float)Math.Abs(c.X - d.X);
               float xM = x1 > x2 ? x1 : x2;
               float xPT = xM > 0 ? (intpt.X - a.X) / xM : 0;   //gives us percentage IN THE TILE

               float z1 = (float)Math.Abs(a.Z - c.Z);
               float z2 = (float)Math.Abs(b.Z - d.Z);
               float zM = z1 > z2 ? z1 : z2;
               float zPT = zM > 0 ? (intpt.Z - a.Z) / zM : 0;   //gives us percentage IN THE TILE

               //scale that up to percentages in the space of our pixels
               mDecalInstances[i].mTileCenter.X = ((xPT * vertsToHighResPixelSpaceRatio) + (myTileX * vertsToHighResPixelSpaceRatio));
               mDecalInstances[i].mTileCenter.Y = ((zPT * vertsToHighResPixelSpaceRatio) + (myTileZ * vertsToHighResPixelSpaceRatio));

               recomputeDecalInstanceBounds(i, true);
            }
         }
      }
      public void recomputeDecalInstanceBounds(int activeDecalInstanceIndex, bool redistributeToQNs)
      {
         //CLM THIS MATH IS VERY TRICKY!! PLEASE TALK TO ME BEFORE SCREWING WITH IT!!!!
         //CLM THIS MATH IS VERY TRICKY!! PLEASE TALK TO ME BEFORE SCREWING WITH IT!!!!

         //grab a list of previous QNs that we affected
         List<BTerrainQuadNode> oldNodes = giveNodesIntersectingDecalInstance(activeDecalInstanceIndex);


         BTerrainDecalInstance dcli = mDecalInstances[activeDecalInstanceIndex];
         BTerrainActiveDecalContainer dcl = mActiveDecals[dcli.mActiveDecalIndex];

         dcli.computeBounds();


         if (redistributeToQNs)
         {
            List<BTerrainQuadNode> newNodes = giveNodesIntersectingDecalInstance(activeDecalInstanceIndex);
            for (int i = 0; i < newNodes.Count; i++)
            {
               //does this container already contain this decal reference?
               if (!newNodes[i].mLayerContainer.containsID(activeDecalInstanceIndex, BTerrainTexturingLayer.eLayerType.cLayer_Decal))
               {
                  if (newNodes[i].mLayerContainer.getNumDecalLayers() + 1 >= BTerrainTexturing.cMaxDecalsPerChunk)
                  {
                     MessageBox.Show("Error: This decal operation exceeds the max number of decals for one or more chunks.\n Please remove decals to get below the " + BTerrainTexturing.cMaxDecalsPerChunk + " max value.");
                     continue;
                  }
                  newNodes[i].mLayerContainer.newDecalLayer(activeDecalInstanceIndex);
                  int index = newNodes[i].mLayerContainer.giveLayerIndex(activeDecalInstanceIndex, BTerrainTexturingLayer.eLayerType.cLayer_Decal);
                  newNodes[i].mLayerContainer.computeDecalLayerAlphaContrib(index, newNodes[i].getDesc().mMinXVert, newNodes[i].getDesc().mMinZVert);
               }
               else
               {
                  int index = newNodes[i].mLayerContainer.giveLayerIndex(activeDecalInstanceIndex, BTerrainTexturingLayer.eLayerType.cLayer_Decal);
                  newNodes[i].mLayerContainer.computeDecalLayerAlphaContrib(index, newNodes[i].getDesc().mMinXVert, newNodes[i].getDesc().mMinZVert);
               }

               //newNodes[i].getTextureData().free();
               for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
               {
                  newNodes[i].getTextureData(lod).free();
               }
            }

            //remove references from any older nodes that don't use us.
            for (int i = 0; i < oldNodes.Count; i++)
            {
               if (!newNodes.Contains(oldNodes[i]))
               {
                  int index = oldNodes[i].mLayerContainer.giveLayerIndex(activeDecalInstanceIndex, BTerrainTexturingLayer.eLayerType.cLayer_Decal);
                  if (index != -1)
                     oldNodes[i].mLayerContainer.removeLayer(index);

                  //oldNodes[i].getTextureData().free();
                  for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
                  {
                     oldNodes[i].getTextureData(lod).free();
                  }
               }

            }
            newNodes.Clear();
            newNodes = null;
         }
         oldNodes.Clear();
         oldNodes = null;
      }
   }
}
