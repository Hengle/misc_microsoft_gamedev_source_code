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
using System.Diagnostics;

using Rendering;
using EditorCore;
using SimEditor;


 
//-----------------------------------------------
namespace Terrain
{

   //------------------------------------------------------------
   //------------------------------------------------------------
   //------------------------------------------------------------
   //these are used by the quadnode to edit our layer data
   public class BTerrainTexturingLayer
   {
      ~BTerrainTexturingLayer()
      {
         mAlphaLayer = null;
      }
      public enum eLayerType
      {
         cLayer_Splat = 0,
         cLayer_Decal,
      }
      public eLayerType mLayerType;     //0=ActiveTexture, 1=staticDecal
      public int mActiveTextureIndex;
      public byte[] mAlphaLayer;

      public void copyTo(BTerrainTexturingLayer target)
      {
         target.mLayerType = mLayerType;
         target.mActiveTextureIndex = mActiveTextureIndex;
         if (target.mAlphaLayer == null)
            target.mAlphaLayer = new Byte[BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight()];
         mAlphaLayer.CopyTo(target.mAlphaLayer, 0);
      }

      public int getMemorySize()
      {
         return sizeof(eLayerType) + sizeof(int) + mAlphaLayer.Length;
      }
   }

   public class BTerrainLayerContainer
   {
      public BTerrainLayerContainer()
      {
      }
      ~BTerrainLayerContainer()
      {
      }
      public void destroy()
      {
         clearAllLayers();
      }

      //Functions which will effect both layer types
      public int getNumLayers()
      {
         return mLayers.Count;
      }
      public int getNumNonBlankLayers()
      {
         int c = 0;
         for (int i = 0; i < mLayers.Count; i++)
         {
            if (!isLayerAllClear(i))
               c++;
         }
         return c;
      }
      public void clearAllLayers()
      {
         for (int i = 0; i < mLayers.Count; i++)
            mLayers[i].mAlphaLayer = null;

         mLayers.Clear();
      }
      public void activeIndexRemovedCascade(int index, BTerrainTexturingLayer.eLayerType layerType)
      {
         //cascade
         for (int i = 0; i < mLayers.Count; i++)
         {
            if (mLayers[i].mActiveTextureIndex >= index && mLayers[i].mLayerType == layerType)
            {
               mLayers[i].mActiveTextureIndex--;
            }
         }
      }
      public void removeLayersWithWrongID()
      {
         //layer 0 is always a splat
         if (mLayers.Count == 1 && mLayers[0].mActiveTextureIndex >= TerrainGlobals.getTexturing().getActiveTextureCount())
         {
            mLayers[0].mActiveTextureIndex = 0;
            return;
         }

         for (int i = 0; i < mLayers.Count; i++)
         {
            if ((mLayers[i].mActiveTextureIndex >= TerrainGlobals.getTexturing().getActiveTextureCount() && mLayers[i].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat) ||
                (mLayers[i].mActiveTextureIndex >= TerrainGlobals.getTexturing().getActiveDecalInstancesCount() && mLayers[i].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal))
            {
               clearLayerAlpha(i);
               mLayers.RemoveAt(i);
               i--;
            }
         }

         fullLayerAlpha(0);
      }
      public void removeLayersWithTexID(int texID, BTerrainTexturingLayer.eLayerType layerType, bool topLayerOnly)
      {
         //layer 0 is always a splat
         if (mLayers.Count == 1)
         {
            mLayers[0].mActiveTextureIndex = 0;
            return;
         }

         for (int i = 0; i < mLayers.Count; i++)
         {
            if (mLayers[i].mActiveTextureIndex == texID && mLayers[i].mLayerType == layerType)
            {
               clearLayerAlpha(i);
               mLayers.RemoveAt(i);
               i--;
               if (topLayerOnly)
                  return;
            }
         }

         fullLayerAlpha(0);

      }
      public void removeLayer(int layerIndex)
      {
         mLayers.RemoveAt(layerIndex);
         fullLayerAlpha(0);
      }
      public void removeLayersBelow(int layerIndex)
      {
         if (layerIndex == 0)
            return;

         mLayers.RemoveRange(0, layerIndex);
         fullLayerAlpha(0);
      }
      public void replaceIDWithID(int origTexID, int newTexID, bool topLayerOnly)
      {
         for (int i = 0; i < mLayers.Count; i++)
         {
            if (mLayers[i].mActiveTextureIndex == origTexID)
            {
               mLayers[i].mActiveTextureIndex = newTexID;
               if (topLayerOnly)
                  return;
            }
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
      public BTerrainTexturingLayer giveLayer(int layerIndex)
      {
         if (layerIndex < 0 || layerIndex >= mLayers.Count)
            return null;

         return mLayers[layerIndex];
      }
      public BTerrainTexturingLayer giveTopmostLayer()
      {
         return giveLayer(mLayers.Count - 1);
      }
      public void clearLayerAlpha(int layerIndex)
      {
         if (layerIndex < 0 || layerIndex >= mLayers.Count)
            return;

         for (int x = 0; x < (int)BTerrainTexturing.getAlphaTextureWidth(); x++)
         {
            for (int z = 0; z < (int)BTerrainTexturing.getAlphaTextureHeight(); z++)
            {
               mLayers[layerIndex].mAlphaLayer[x + BTerrainTexturing.getAlphaTextureWidth() * z] = 0;
            }
         }
      }
      public void fullLayerAlpha(int layerIndex)
      {
         if (layerIndex < 0 || layerIndex >= mLayers.Count)
            return;

         for (int x = 0; x < (int)BTerrainTexturing.getAlphaTextureWidth(); x++)
         {
            for (int z = 0; z < (int)BTerrainTexturing.getAlphaTextureHeight(); z++)
            {
               mLayers[layerIndex].mAlphaLayer[x + BTerrainTexturing.getAlphaTextureWidth() * z] = Byte.MaxValue;
            }
         }
      }
      public void removeBlankLayers()
      {
         //for (int i = 0; i < mLayers.Count; i++)
         //{
         //   if (isLayerAllClear(i))
         //   {
         //      removeLayer(i);
         //      i--;
         //   }
         //}

         //fullLayerAlpha(0);
      }
      public void removeHiddenLayers()
      {
         int sze = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());
         //remove layers below a fully opaque layer
         for (int i = 1; i < mLayers.Count; i++)
         {
            if (mLayers[i].mLayerType != BTerrainTexturingLayer.eLayerType.cLayer_Splat)
               continue;

            if (isLayerAllSolid(i))
            {
               removeLayersBelow(i);
               i = 0;
            }
         }
      }
      public bool isLayerAllSolid(int layerIndex)
      {
         if (layerIndex < 0 || layerIndex >= mLayers.Count)
            return false;

         for (int x = 0; x < (int)BTerrainTexturing.getAlphaTextureWidth(); x++)
         {
            for (int z = 0; z < (int)BTerrainTexturing.getAlphaTextureHeight(); z++)
            {
               if (mLayers[layerIndex].mAlphaLayer[x + BTerrainTexturing.getAlphaTextureWidth() * z] != Byte.MaxValue)
                  return false;
            }
         }
         return true;
      }
      public bool isLayerAllClear(int layerIndex)
      {
         if (layerIndex < 0 || layerIndex >= mLayers.Count)
            return false;

         for (int x = 0; x < (int)BTerrainTexturing.getAlphaTextureWidth(); x++)
         {
            for (int z = 0; z < (int)BTerrainTexturing.getAlphaTextureHeight(); z++)
            {
               if (mLayers[layerIndex].mAlphaLayer[x + BTerrainTexturing.getAlphaTextureWidth() * z] != 0)
                  return false;
            }
         }
         return true;
      }

      private int giveLayerIDAtPixelInternal(int x, int y, int startingLayer, ref BTerrainTexturingLayer.eLayerType selectedLayerType)
      {
         if (startingLayer > mLayers.Count - 1)
         {
            startingLayer = mLayers.Count - 1;
         }

         if (startingLayer < 0)
         {
            selectedLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
            return 0;
         }

         for (int i = startingLayer; i >= 0; i--)
         {
            if (mLayers[i].mAlphaLayer[x + BTerrainTexturing.getAlphaTextureWidth() * y] != 0)
            {
               if (mLayers[i].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
               {
                  selectedLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
                  return mLayers[i].mActiveTextureIndex;
               }
               else
               {
                  BTerrainDecalInstance dcli = TerrainGlobals.getTexturing().getActiveDecalInstance(mLayers[i].mActiveTextureIndex);
                  if (TerrainGlobals.getEditor().mVisTileIntetersectionX <= dcli.mTileBoundsMax.X && TerrainGlobals.getEditor().mVisTileIntetersectionX >= dcli.mTileBoundsMin.X &&
                     TerrainGlobals.getEditor().mVisTileIntetersectionZ <= dcli.mTileBoundsMax.Y && TerrainGlobals.getEditor().mVisTileIntetersectionZ >= dcli.mTileBoundsMin.Y)
                  {
                     selectedLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Decal;
                     return i;// dcli.mActiveDecalIndex;
                  }
               }
            }
         }

         selectedLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
         return -1;
      }
      public int giveDecalLayerIDAtPixel(int x, int y, bool doSelect)
      {
         BTerrainTexturingLayer.eLayerType reftype = BTerrainTexturingLayer.eLayerType.cLayer_Decal;
         int index = 0;
         int start = mLayers.Count - 1;
         while (reftype != BTerrainTexturingLayer.eLayerType.cLayer_Decal)
         {
            index = giveLayerIDAtPixelInternal(x, y, start, ref reftype);
            if (index == -1)
               return 0;
         }

         if (doSelect)
            TerrainGlobals.getTexturing().selectActiveDecalInstance(mLayers[index].mActiveTextureIndex);

         return index;

      }
      public int giveSplatLayerIDAtPixel(int x, int y)
      {
         BTerrainTexturingLayer.eLayerType reftype = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
         int index = 0;
         int start = mLayers.Count - 1;
         while (reftype != BTerrainTexturingLayer.eLayerType.cLayer_Splat)
         {
            index = giveLayerIDAtPixelInternal(x, y, start, ref reftype);
            if (index == -1)
               return 0;
         }

         return index;
      }
      public int giveLayerIDAtPixel(int x, int y, ref BTerrainTexturingLayer.eLayerType selectedLayerType, bool doSelectDecal)
      {
         int index = giveLayerIDAtPixelInternal(x, y, mLayers.Count - 1, ref selectedLayerType);
         if (index == -1)
         {
            return 0;
         }

         if (selectedLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal && doSelectDecal)
         {
            TerrainGlobals.getTexturing().selectActiveDecalInstance(mLayers[index].mActiveTextureIndex);
            return mLayers[index].mActiveTextureIndex;
         }

         return index;

         /*
         for (int i = mLayers.Count - 1; i >= 0; i--)
         {

            if (mLayers[i].mAlphaLayer[x + BTerrainTexturing.getAlphaTextureWidth() * y] != 0)
            {
               if (mLayers[i].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
               {
                  selectedLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
                  return mLayers[i].mActiveTextureIndex;
               }
               else
               {
                  BTerrainDecalInstance dcli = TerrainGlobals.getTexturing().getActiveDecalInstance(mLayers[i].mActiveTextureIndex);
                  if (TerrainGlobals.getEditor().mVisTileIntetersectionX <= dcli.mTileBoundsMax.X && TerrainGlobals.getEditor().mVisTileIntetersectionX >= dcli.mTileBoundsMin.X &&
                     TerrainGlobals.getEditor().mVisTileIntetersectionZ <= dcli.mTileBoundsMax.Y && TerrainGlobals.getEditor().mVisTileIntetersectionZ >= dcli.mTileBoundsMin.Y)
                  {
                     if (doSelectDecal)
                        TerrainGlobals.getTexturing().selectActiveDecalInstance(mLayers[i].mActiveTextureIndex);

                     selectedLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Decal;
                     return dcli.mActiveDecalIndex;
                  }
               }
            }
         }

         selectedLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
         return mLayers[0].mActiveTextureIndex;
          * */
      }

      //used for clipart
      public BTerrainTextureVector giveLayerChainAtPixel(int x, int z)
      {
         BTerrainTextureVector v = new BTerrainTextureVector();
         if (x < 0 || z < 0 || x >= BTerrainTexturing.getAlphaTextureWidth() || z >= BTerrainTexturing.getAlphaTextureHeight())
            return v;

         for (int i = 0; i < mLayers.Count; i++)
         {
            BTerrainPerVertexLayerData layer = new BTerrainPerVertexLayerData();
            layer.mActiveTextureIndex = mLayers[i].mActiveTextureIndex;
            layer.mLayerType = mLayers[i].mLayerType;
            layer.mAlphaContrib = mLayers[i].mAlphaLayer[x + BTerrainTexturing.getAlphaTextureWidth() * z];
            v.mLayers.Add(layer);
         }

         return v;
      }
      public void clearAllAlphasAtPixel(int x, int z)
      {
         if (x < 0 || z < 0 || x >= BTerrainTexturing.getAlphaTextureWidth() || z >= BTerrainTexturing.getAlphaTextureHeight())
            return;

         for (int i = mLayers.Count - 1; i >= 0; i--)
         {
            mLayers[i].mAlphaLayer[x + BTerrainTexturing.getAlphaTextureWidth() * z] = 0;
         }
      }
      public void append(BTerrainLayerContainer target)
      {
         //let's just take the values here and add them to ourselves..
         for (int i = 0; i < target.getNumLayers(); i++)
         {
            BTerrainTexturingLayer lyr = new BTerrainTexturingLayer();
            target.mLayers[i].copyTo(lyr);
            mLayers.Add(lyr);
         }
      }
      public void removeAppendedLayers(int startIndex)
      {
         int eIdx = (mLayers.Count) - startIndex;
         mLayers.RemoveRange(startIndex, eIdx);
      }
      public void merge(BTerrainLayerContainer target)
      {
         //add the layers, then reduce
         append(target);
         removeRedundantLayers();
         /*
         //walk our layers
         for(int i=0;i<target.getNumLayers();i++)
         {
            int indx = giveLayerIndex(target.mLayers[i].mActiveTextureIndex);
            if(indx == -1)
            {
            }
            else
            {
               //subtract all values between us and them.
            }
         }*/
      }

      //Functions will effect Splat Only
      public int newSplatLayer(int activeTextureIndex)
      {
         return newSplatLayer(activeTextureIndex, true);
      }
      public int newSplatLayer(int activeTextureIndex, bool insertKeepOrdering)
      {
         if (mLayers.Count > 0
            && mLayers[mLayers.Count - 1].mActiveTextureIndex == activeTextureIndex
            && mLayers[mLayers.Count - 1].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
            return mLayers.Count - 1;

         int addedIndex = -1;
         if (insertKeepOrdering)
         {
            for (int i = 0; i < mLayers.Count; i++)
            {
               if (mLayers[i].mActiveTextureIndex > activeTextureIndex)
               {
                  addedIndex = i;
                  break;
               }
            }
         }
        

         if (addedIndex == - 1)
         {
            mLayers.Add(new BTerrainTexturingLayer());
            addedIndex=mLayers.Count - 1;
         }
         else
         {
            mLayers.Insert(addedIndex, new BTerrainTexturingLayer());
         }

         mLayers[addedIndex].mLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
         mLayers[addedIndex].mActiveTextureIndex = activeTextureIndex;
         mLayers[addedIndex].mAlphaLayer = new Byte[BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight()];


         return addedIndex;
      }
      public int getNumSplatLayers()
      {
         int count = 0;
         for (int i = 0; i < mLayers.Count; i++)
            if (mLayers[i].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
               count++;
         return count;
      }
      public void makeBaseLayer(int baseLayerTextureIndex)
      {
         if (mLayers.Count == 0)
         {
            newSplatLayer(baseLayerTextureIndex);
         }
         else
         {
            mLayers[0].mActiveTextureIndex = baseLayerTextureIndex;
         }
         fullLayerAlpha(0);
      }
      private void compactLayers(int bottomLayerIndex, int topLayerIndex)
      {
         int sze = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());

         //subtract from middle layers first
         BTerrainTexturingLayer topLayer = giveLayer(topLayerIndex);
         for (int i = bottomLayerIndex + 1; i < topLayerIndex; i++)
         {
            BTerrainTexturingLayer middleLayer = giveLayer(i);
            //  if (middleLayer.mLayerType != BTerrainTexturingLayer.eLayerType.cLayer_Splat)
            //     continue;

            for (int x = 0; x < sze; x++)
            {
               middleLayer.mAlphaLayer[x] = (byte)(BMathLib.Clamp(middleLayer.mAlphaLayer[x] - topLayer.mAlphaLayer[x], 0, 255));
            }
         }

         //now add it to the bottom layer
         BTerrainTexturingLayer bottomLayer = giveLayer(bottomLayerIndex);
         for (int x = 0; x < sze; x++)
         {
            bottomLayer.mAlphaLayer[x] = (byte)(BMathLib.Clamp(bottomLayer.mAlphaLayer[x] + topLayer.mAlphaLayer[x], 0, 255));
         }


         //remove top layer
         removeLayer(topLayerIndex);
      }
      public void removeRedundantLayers()
      {
         for (int i = 0; i < getNumLayers(); i++)
         {
            BTerrainTexturingLayer topLayer = giveLayer(i);
            if (topLayer.mLayerType != BTerrainTexturingLayer.eLayerType.cLayer_Splat)
               continue;
            //find another layer with my same ID;
            int idToFind = topLayer.mActiveTextureIndex;
            for (int q = i + 1; q < getNumLayers(); q++)
            {
               if (giveLayer(q).mActiveTextureIndex == idToFind && giveLayer(q).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
               {
                  compactLayers(i, q);
               }
            }
         }
      }


      //Functions will effect Decal Only
      public int newDecalLayer(int activeDecalInstanceIndex)
      {
         if (mLayers.Count > 0
               && mLayers[mLayers.Count - 1].mActiveTextureIndex == activeDecalInstanceIndex
               && mLayers[mLayers.Count - 1].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
            return mLayers.Count - 1;

         mLayers.Add(new BTerrainTexturingLayer());

         mLayers[mLayers.Count - 1].mLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Decal;
         mLayers[mLayers.Count - 1].mActiveTextureIndex = activeDecalInstanceIndex;
         mLayers[mLayers.Count - 1].mAlphaLayer = new Byte[BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight()];


         fullLayerAlpha(mLayers.Count - 1);
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
      public void computeDecalLayerAlphaContrib(int layerIndex, int minXVertex, int minZVertex)
      {

         if (layerIndex < 0 || layerIndex >= mLayers.Count)
            return;

         //grab our decal instance
         if (mLayers[layerIndex].mLayerType != BTerrainTexturingLayer.eLayerType.cLayer_Decal)
            return;

         float vertsToHighResPixelSpaceRatio = BTerrainTexturing.getTextureWidth() / BTerrainQuadNode.cMaxWidth;

         BTerrainDecalInstance dcli = TerrainGlobals.getTexturing().getActiveDecalInstance(mLayers[layerIndex].mActiveTextureIndex);
         BTerrainActiveDecalContainer dcl = TerrainGlobals.getTexturing().getActiveDecal(dcli.mActiveDecalIndex);


         //scale, rotate, translate us
         int h = dcl.mHeight;
         int w = dcl.mWidth;
         int nW = (int)(w * ((1.0f / dcli.mUScale) / vertsToHighResPixelSpaceRatio));
         int nH = (int)(h * ((1.0f / dcli.mVScale) / vertsToHighResPixelSpaceRatio));
         int tW = 0;
         int tH = 0;
         byte[] tImgResized = null;
         byte[] tImgRotated = null;
         byte[] imgDat = new byte[h * w];
         imgDat = ImageManipulation.valueGreyScaleImg(imgDat, w, h, ImageManipulation.eValueOperation.cValue_Set, 255);
         tImgResized = ImageManipulation.resizeGreyScaleImg(imgDat, w, h, nW, nH, ImageManipulation.eFilterType.cFilter_Linear);
         tImgRotated = ImageManipulation.rotateGreyScaleImg(tImgResized, nW, nH, (float)(-dcli.mRotation * (180.0f / Math.PI)), false, out nW, out nH, ImageManipulation.eFilterType.cFilter_Nearest);

         byte[] tImg = tImgRotated;
         int startX = (int)((dcli.mTileCenter.X / vertsToHighResPixelSpaceRatio) - (nW >> 1) - minXVertex);
         int startY = (int)((dcli.mTileCenter.Y / vertsToHighResPixelSpaceRatio) - (nH >> 1) - minZVertex);

         clearLayerAlpha(layerIndex);

         //copy back to masking
         for (int i = 0; i < nW; i++)
         {
            for (int j = 0; j < nH; j++)
            {
               int iS = startX + i;
               if (iS < 0 || iS >= BTerrainTexturing.getAlphaTextureWidth())
                  continue;

               int jS = startY + j;
               if (jS < 0 || jS >= BTerrainTexturing.getAlphaTextureHeight())
                  continue;

               int srcIndex = j + i * nW;
               int dstIndex = (int)(iS + BTerrainTexturing.getAlphaTextureHeight() * jS);

               byte val = tImg[srcIndex];
               mLayers[layerIndex].mAlphaLayer[dstIndex] = val;


            }
         }

         //also, walk any layers that exist above us, and subtract out their values as well
         for (int k = layerIndex + 1; k < mLayers.Count; k++)
         {
            if (mLayers[k].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
            {
               for (int i = 0; i < BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight(); i++)
                  mLayers[layerIndex].mAlphaLayer[i] = (byte)BMathLib.Clamp(mLayers[layerIndex].mAlphaLayer[i] - mLayers[k].mAlphaLayer[i], 0, 255);
            }
         }

         imgDat = null;
         tImgResized = null;
         tImgRotated = null;
         tImg = null;
      }

      void fillCreateLayer(byte[] inputImg, int activeTexIndex, BTerrainTexturingLayer.eLayerType type, int minXTile, int minZTile, byte[] tempLargerImge, byte[] tempBorderedImg)
      {
         int width = (int)BTerrainTexturing.getAlphaTextureWidth();
         int height = (int)BTerrainTexturing.getAlphaTextureHeight();

         int lw = width + 2;
         int lh = height + 2;
         //Create our texture first, border it, then resize it.
         //fill the origional texture into the new texture
         for (int q = 0; q < (width); q++)
         {
            for (int j = 0; j < (height); j++)
            {
               tempLargerImge[(q + 1) + lw * (j + 1)] = inputImg[q + (width) * j];
            }
         }

         int numXNodes = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
         int rootXNode = (int)(minXTile / BTerrainQuadNode.cMaxWidth);
         int rootZNode = (int)(minZTile / BTerrainQuadNode.cMaxHeight);

         BTerrainQuadNode rootNode = TerrainGlobals.getTerrain().getLeafNode(rootXNode,rootZNode);//TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafNodeContainingTile(minXTile, minZTile);
         BTerrainQuadNode node = null;
         int layerIndex = 0;
         BTerrainTexturingLayer lyr = null;



         #region SIDES
         //grab neighbor alpha values, paste them into my edges.
         //LEFT SIDE FILL
         node = TerrainGlobals.getTerrain().getLeafNode(rootXNode-1, rootZNode);// rootNode.getNeighborNode(-1, 0);
         if ((node != null) && 
            ((layerIndex = node.mLayerContainer.giveLayerIndex(activeTexIndex, type)) !=-1))
         {
            {
               lyr = node.mLayerContainer.giveLayer(layerIndex);
               //grab the RIGHT pixels
               for (int i = 0; i < height; i++)
               {
                  int srcIndex = (width - 1) + width * i;
                  int destIndex = 0 + (lw * (i + 1));
                  tempLargerImge[destIndex] = lyr.mAlphaLayer[srcIndex];
               }
            }
         }
         else
         {
            //extend the LEFT pixels
            for (int i = 0; i < height; i++)
            {
               int srcIndex = 0 + width * i;
               int destIndex = 0 + (lw * (i + 1));
               tempLargerImge[destIndex] = inputImg[srcIndex];
            }
         }


         //RIGHT SIDE FILL
         node = TerrainGlobals.getTerrain().getLeafNode(rootXNode+1, rootZNode);// rootNode.getNeighborNode(1, 0);
         if ((node != null) && 
            ((layerIndex = node.mLayerContainer.giveLayerIndex(activeTexIndex, type)) !=-1))
         {
            lyr = node.mLayerContainer.giveLayer(layerIndex);
            //grab the LEFT pixels
            for (int i = 0; i < height; i++)
            {
               int srcIndex = 0 + width * i;
               int destIndex = (lw - 1) + (lw * (i + 1));
               tempLargerImge[destIndex] = lyr.mAlphaLayer[srcIndex];
            }
         }
         else
         {
            
               //extend the RIGHT pixels
               for (int i = 0; i < height; i++)
               {
                  int srcIndex = (width - 1) + width * i;
                  int destIndex = (lw - 1) + (lw * (i + 1));
                  tempLargerImge[destIndex] = inputImg[srcIndex];
               }
            
         }

         //TOP SIDE FILL
         node = TerrainGlobals.getTerrain().getLeafNode(rootXNode, rootZNode + 1);// rootNode.getNeighborNode(0, 1);
         if ((node != null) && 
            ((layerIndex = node.mLayerContainer.giveLayerIndex(activeTexIndex, type)) !=-1))
         {
            {
               lyr = node.mLayerContainer.giveLayer(layerIndex);
               //grab the BOTTOM pixels
               for (int i = 0; i < width; i++)
               {
                  int srcIndex = i + width * 0;
                  int destIndex = (i + 1) + (lw * (lh - 1));
                  tempLargerImge[destIndex] = lyr.mAlphaLayer[srcIndex];
               }
            }
         }
         else
         {
            //extend the TOP pixels
            for (int i = 0; i < width; i++)
            {
               int srcIndex = i + width * (height - 1);
               int destIndex = (i + 1) + (lw * (lh - 1));
               tempLargerImge[destIndex] = inputImg[srcIndex];
            }
         }

         //BOTTOM SIDE FILL
         node = TerrainGlobals.getTerrain().getLeafNode(rootXNode, rootZNode - 1);// rootNode.getNeighborNode(0, -1);
         if ((node != null) && 
            ((layerIndex = node.mLayerContainer.giveLayerIndex(activeTexIndex, type)) !=-1))
         {
            {
               lyr = node.mLayerContainer.giveLayer(layerIndex);
               //grab the TOP pixels
               for (int i = 0; i < width; i++)
               {
                  int srcIndex = i + width * (height - 1);
                  int destIndex = (i + 1) + (lw * 0);
                  tempLargerImge[destIndex] = lyr.mAlphaLayer[srcIndex];
               }
            }
         }
         else
         {
            //extend the BOTTOM pixels
            for (int i = 0; i < width; i++)
            {
               int srcIndex = i + width * 0;
               int destIndex = (i + 1) + (0);
               tempLargerImge[destIndex] = inputImg[srcIndex];
            }
         }
         #endregion

         #region CORNERS
         //TOP LEFT CORNER
         node = TerrainGlobals.getTerrain().getLeafNode(rootXNode-1, rootZNode+ 1);//rootNode.getNeighborNode(-1, 1);
         if ((node != null) && 
            ((layerIndex = node.mLayerContainer.giveLayerIndex(activeTexIndex, type)) !=-1))
         {
            {
               lyr = node.mLayerContainer.giveLayer(layerIndex);
               //grab the BOTTOM RIGHT pixel
               //for (int i = 0; i < width; i++)
               {
                  int srcIndex = (width - 1) + width * 0;
                  int destIndex = 0 + (lw * (lh - 1));
                  tempLargerImge[destIndex] = lyr.mAlphaLayer[srcIndex];
               }
            }
         }
         else
         {
            //grab the tpo+1, left+1 pixel
            //for (int i = 0; i < width; i++)
            {
               int srcIndex = 1 + width * (height - 2);
               int destIndex = 0 + (lw * (lh - 1));
               tempLargerImge[destIndex] = inputImg[srcIndex];
            }
         }
         //TOP RIGHT CORNER
         node = TerrainGlobals.getTerrain().getLeafNode(rootXNode + 1, rootZNode + 1);//rootNode.getNeighborNode(1, 1);
         if ((node != null) &&
            ((layerIndex = node.mLayerContainer.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            {
               lyr = node.mLayerContainer.giveLayer(layerIndex);
               //grab the BOTTOM LEFT pixel
               //for (int i = 0; i < width; i++)
               {
                  int srcIndex = 0;// +width * (height - 1);
                  int destIndex = (lw - 1) + (lw * (lh - 1));
                  tempLargerImge[destIndex] = lyr.mAlphaLayer[srcIndex];
               }
            }
           
         }
         else
         {
           
               //grab the TOP+1 RIGHT+1 pixel
               //for (int i = 0; i < width; i++)
               {
                  int srcIndex = (width - 2) + width * (height - 2);
                  int destIndex = (lw - 1) + (lw * (lh - 1));
                  tempLargerImge[destIndex] = inputImg[srcIndex];
               }
            
         }
         //BOTTOM LEFT CORNER
         node = TerrainGlobals.getTerrain().getLeafNode(rootXNode - 1, rootZNode - 1);//rootNode.getNeighborNode(-1, -1);
         if ((node != null) &&
            ((layerIndex = node.mLayerContainer.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            {
               lyr = node.mLayerContainer.giveLayer(layerIndex);
               //grab the TOP RIGHT pixel
               //for (int i = 0; i < width; i++)
               {
                  int srcIndex = (width - 1) + width * (height - 1);
                  int destIndex = 0 + (lw * 0);
                  tempLargerImge[destIndex] = lyr.mAlphaLayer[srcIndex];
               }
            }
           
         }
         else
         {
            //grab the BOTTOM+1 LEFT+1 pixel
            //for (int i = 0; i < width; i++)
            {
               int srcIndex = 1 + width * 1;
               int destIndex = 0 + (lw * 0);
               tempLargerImge[destIndex] = inputImg[srcIndex];
            }
         }
         //BOTTOM RIGHT CORNER
         node = TerrainGlobals.getTerrain().getLeafNode(rootXNode + 1, rootZNode - 1);//rootNode.getNeighborNode(1, -1);
         if ((node != null) &&
            ((layerIndex = node.mLayerContainer.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            {
               lyr = node.mLayerContainer.giveLayer(layerIndex);
               //grab the TOP LEFT pixel
               //for (int i = 0; i < width; i++)
               {
                  int srcIndex = (0) + width * (height - 1);
                  int destIndex = (lw - 1) + (lw * 0);
                  tempLargerImge[destIndex] = lyr.mAlphaLayer[srcIndex];
               }
            }
           
         }
         else
         {
            //grab the BOTTOM+1 RIGHT+1 pixel
            //for (int i = 0; i < width; i++)
            {
               int srcIndex = (width - 2) + width * 1;
               int destIndex = (lw - 1) + (lw * 0);
               tempLargerImge[destIndex] = inputImg[srcIndex];
            }
         }

         #endregion

         ImageManipulation.resizeGreyScaleImg(tempLargerImge, tempBorderedImg, width + 2, height + 2, width, height, ImageManipulation.eFilterType.cFilter_Linear);
      }

      public void toTextureArray(ref List<Texture> mTempAlphaTextures, int minXVert, int minZVert)
      {
         bool doBlendedFill = true;

         //lock in our alpha texture
         int slicePitch = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());
         int count = Math.Min(mTempAlphaTextures.Count, mLayers.Count);

         int width = (int)BTerrainTexturing.getAlphaTextureWidth();
         int height = (int)BTerrainTexturing.getAlphaTextureHeight();

         byte[] tempLargerImg = new byte[(width + 2) * (height + 2)];
         byte[] bordered = new byte[width * height];

         int i = 0;
         for (i = 0; i < count; i++)
         {
            if (mTempAlphaTextures[i] != null)
            {
               GraphicsStream texstream = mTempAlphaTextures[i].LockRectangle(0, LockFlags.None);
               if(i==0)
               {
                  for(int k=0;k<slicePitch;k++)
                     texstream.WriteByte(255);
               }
               else
               {
                  if (doBlendedFill)
                  {
                     fillCreateLayer(mLayers[i].mAlphaLayer, mLayers[i].mActiveTextureIndex, mLayers[i].mLayerType, minXVert, minZVert, tempLargerImg, bordered);
                     texstream.Write(bordered, 0, slicePitch);
                  }
                  else
                  {
                     texstream.Write(mLayers[i].mAlphaLayer, 0, slicePitch);
                  }

               }
               
               mTempAlphaTextures[i].UnlockRectangle(0);
               texstream.Close();

            }
            else
            {

            }
         }

         //we've got more layers than we've preallocated
         if (mTempAlphaTextures.Count < mLayers.Count)
         {
            int diff = mLayers.Count - mTempAlphaTextures.Count;
            for (int k = 0; k < diff; k++)
            {
               mTempAlphaTextures.Add(new Texture(BRenderDevice.getDevice(), (int)BTerrainTexturing.getAlphaTextureWidth(), (int)BTerrainTexturing.getAlphaTextureHeight(), 1, 0, Format.L8, Pool.Managed));

               GraphicsStream texstream = mTempAlphaTextures[mTempAlphaTextures.Count - 1].LockRectangle(0, LockFlags.None);
               if (doBlendedFill)
               {
                  fillCreateLayer(mLayers[i + k].mAlphaLayer, mLayers[i + k].mActiveTextureIndex, mLayers[i + k].mLayerType, minXVert, minZVert, tempLargerImg, bordered);
                  texstream.Write(bordered, 0, slicePitch);
               }
               else
               {
                  texstream.Write(mLayers[i + k].mAlphaLayer, 0, slicePitch);
               }

               mTempAlphaTextures[mTempAlphaTextures.Count - 1].UnlockRectangle(0);
               texstream.Close();
            }
         }

         tempLargerImg = null;
      }

      public void copyTo(ref BTerrainLayerContainer dest)
      {
         if (dest == null)
            dest = new BTerrainLayerContainer();

         dest.mLayers.Clear();
         for (int i = 0; i < mLayers.Count; i++)
         {
            dest.mLayers.Add(new BTerrainTexturingLayer());
            mLayers[i].copyTo(dest.mLayers[i]);
         }
      }

      //Helper Converters for Constrained Splatting
      public void ensureProperSorting()
      {
         //bubbesort
         for (int k = 0; k < mLayers.Count; k++)
         {
            for (int j = k+1; j < mLayers.Count; j++)
            {
               if(mLayers[j].mActiveTextureIndex < mLayers[k].mActiveTextureIndex)
               {
                  mLayers.Insert(k, mLayers[j]);
                  mLayers.RemoveAt(j+1);
               }
            }
         }
      }
      public void ensureProperNumberLayers()
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
               newSplatLayer(k,false);
            }
         }
      }
      public void ensureProperLayerOrdering()
      {
         ensureProperNumberLayers();
         int numSplatLayers = TerrainGlobals.getTexturing().getActiveTextureCount();
         int aWidth = (int)BTerrainTexturing.getAlphaTextureWidth();
         int aHeight = (int)BTerrainTexturing.getAlphaTextureHeight();

         for(int y = 0; y < aHeight;y++)
         {
            for(int x = 0; x < aWidth;x++)
            {
               int aIndex = x + aWidth * y;

               //create N floatN vectors
               floatN[] layerVecs = new floatN[numSplatLayers];
               for (uint k = 0; k < numSplatLayers; k++)
               {
                  layerVecs[k] = new floatN((uint)numSplatLayers);
                  layerVecs[k][k] = 1;
               }

               //quick early out test to make sure we're not screwing up by re-normalizing..
               floatN nVal = new floatN((uint)numSplatLayers);
               for (uint k = 0; k < numSplatLayers; k++)
               {
                  byte b = mLayers[(int)k].mAlphaLayer[aIndex];
                  float alpha = ((float)b) / 255.0f;
                  nVal[k] = alpha;
               }
               float len = nVal.Length();
               if (len == 1.0f)
                  continue;


               //now, calculate a VContrib vector for the current layering.
               floatN vContrib = layerVecs[0].Clone();//start with 100% base layer
               for (uint k = 1; k < numSplatLayers; k++)
               {
                  byte b =mLayers[(int)k].mAlphaLayer[aIndex] ;
                  int lIdx = mLayers[(int)k].mActiveTextureIndex;
                  float alpha = ((float)b)/255.0f;
                  vContrib = (
                     (layerVecs[lIdx] * alpha) + (vContrib * (1 - alpha))
                     );
               }

               //vContrib now holds at each vector element, the amount the layer is visible to the final pixel
               //although it's not sorted in the current layer ordering
               //so, calculate the xF for the new layer ordering, and store it in a temp var
               floatN vContribSorted = new floatN((uint)numSplatLayers);
               floatN vContribSortedInv = new floatN((uint)numSplatLayers);

               //back solve to calculate new alpha value.
               vContribSorted[(uint)(numSplatLayers - 1)] = vContrib[(uint)(numSplatLayers - 1)];
               vContribSortedInv[(uint)(numSplatLayers - 1)] = 1 - vContribSorted[(uint)(numSplatLayers - 1)];
               for (int k = (numSplatLayers-2); k >= 0; k--)
               {
                  //multiply the inversions together that we have so far.
                  float mulSoFar = 1;
                  for (uint q = (uint)k + 1; q < numSplatLayers; q++)
                     mulSoFar *= vContribSortedInv[q];


                  vContribSorted[(uint)k] = mulSoFar == 0 ? 0 : vContrib[(uint)(k)] / mulSoFar;
                  vContribSortedInv[(uint)k] = 1 - vContribSorted[(uint)k];

                  //uint invK = (uint)((numSplatLayers - 2) - k);
                  //float Vc = vContrib[(uint)(k + 1)];   //vContrib for upper layer
                  //float invVc = 1.0f - Vc;

                  ////current layer vContrib
                  //float cVc = vContrib[(uint)(k)];
                  //float xF = invVc==0?0:(cVc / invVc);

                  ////move back 
                  //vContribSorted[(uint)k] = xF;
               }

               //now copy back to the unsorted layer index
               for (int k = 0; k < numSplatLayers; k++)
               {
                  mLayers[k].mAlphaLayer[aIndex] = (byte)(vContribSorted[(uint)mLayers[k].mActiveTextureIndex] * 255);
               }
            }
         }

         //ensure layers are sorted
         ensureProperSorting();
      }

      //operations on the texture data
      public void mirror(BTerrainFrontend.eMirrorTerrainType type)
      {

      }
      public enum eFlipType
      {
         eFlip_Horiz = 0,
         eFlip_Vert,
      }
      public void flip(eFlipType type)
      {
         int width = (int)(BTerrainTexturing.getAlphaTextureWidth());
         int workWidth = width - 1;
         for (int i = 0; i < mLayers.Count; i++)
         {
            if (type == eFlipType.eFlip_Horiz)
            {
               byte[] tmpRef = (byte[])mLayers[i].mAlphaLayer.Clone();
               for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
               {
                  for (int z = 0; z < BTerrainTexturing.getAlphaTextureWidth(); z++)
                  {
                     int dstIndex = x + width * z;
                     int srcIndex = (workWidth - x) + width * z;

                     mLayers[i].mAlphaLayer[dstIndex] = tmpRef[srcIndex];
                  }
               }
            }
            else if (type == eFlipType.eFlip_Vert)
            {
               byte[] tmpRef = (byte[])mLayers[i].mAlphaLayer.Clone();
               for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
               {
                  for (int z = 0; z < BTerrainTexturing.getAlphaTextureWidth(); z++)
                  {
                     int dstIndex = x + width * z;
                     int srcIndex = x + width * (workWidth - z);

                     mLayers[i].mAlphaLayer[dstIndex] = tmpRef[srcIndex];
                  }
               }
            }
         }
      }
      //used for export 
      public void sort()
      {
         //sort by placing our decals last, and our splats first
         if (getNumDecalLayers() == 0)
            return; //no decals, no problems!

         int lastDecal = mLayers.Count;// -1;
         //walk the list from the back, grabbing decals, placing them a the n-1th spot
         for (int i = mLayers.Count - 1; i >= 0; i--)
         {
            if (mLayers[i].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
            {
               mLayers.Insert(lastDecal, mLayers[i]);
               lastDecal--;
               mLayers.RemoveAt(i);
            }
         }
         //we should be sorted now.
      }
      public int getMemorySize()
      {
         int c = 0;
         for (int i = 0; i < mLayers.Count; i++)
            c += mLayers[i].getMemorySize();

         return c;
      }

      public List<BTerrainTexturingLayer> mLayers = new List<BTerrainTexturingLayer>();

      public bool mIsDirty = false;
   }
}