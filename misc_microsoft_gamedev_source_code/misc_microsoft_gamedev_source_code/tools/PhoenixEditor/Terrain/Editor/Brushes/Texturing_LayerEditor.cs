using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using System.Threading;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
////
using Rendering;
using EditorCore;


//------------------------------------------
namespace Terrain
{
   //------------------------------------------
   //------------------------------------------
   //------------------------------------------
   //------------------------------------------
   class Texturing_LayerEditor
   {

      unsafe static public void newSplatLayerEverywhere(char index)
      {
         BTerrainQuadNode[] nodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
         for (int i = 0; i < nodes.Length; i++)
         {
            if (-1 == nodes[i].mLayerContainer.giveLayerIndex(index, BTerrainTexturingLayer.eLayerType.cLayer_Splat))
               nodes[i].mLayerContainer.newSplatLayer(index);
         }
      }
      unsafe static void valToLayerColumn(BTerrainLayerContainer layers, BTerrainTexturingLayer tLayer, int dIP, byte val, int layerIndex)
      {
         if (val == 0)
            return;

         if (TerrainGlobals.getEditor().mEraseTextureInstead)
         {
            //if i'm erasing, ignore layers above me and just get rid of my values..
            tLayer.mAlphaLayer[dIP] = (byte)(BMathLib.Clamp(tLayer.mAlphaLayer[dIP] - val, 0, 255));
         }
         else
         {
            if (layerIndex + 1 >= layers.getNumLayers())
            {
               //no layers above me
               tLayer.mAlphaLayer[dIP] = (byte)(BMathLib.Clamp(tLayer.mAlphaLayer[dIP] + val, 0, 255));
            }
            else
            {
               float maxByte = 0;
               List<float> vList = new List<float>();
               for (int k = layerIndex + 1; k < layers.getNumLayers(); k++)
               {
                  if (layers.giveLayer(k).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat &&
                      layers.giveLayer(k).mAlphaLayer[dIP] != byte.MinValue)
                  {
                     float v = layers.giveLayer(k).mAlphaLayer[dIP];
                     vList.Add(v);
                     if (v > maxByte) maxByte = v;
                  }
               }

               if (vList.Count == 0)
               {
                  tLayer.mAlphaLayer[dIP] = (byte)(BMathLib.Clamp(tLayer.mAlphaLayer[dIP] + val, 0, 255));
                  return;
               }

               if (tLayer.mAlphaLayer[dIP] < maxByte)
                  tLayer.mAlphaLayer[dIP] = (byte)maxByte;
               


               //normalize the contribs for everything above me
               floatN vec = new floatN((uint)(vList.Count));
               vec.set(vList);
               vec = floatN.Normalize(vec);

               byte hVal = (byte)(val);
               uint T = 0;
               int remainder = 0;
               for (int k = layerIndex + 1; k < layers.getNumLayers(); k++)
               {
                  if (layers.giveLayer(k).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat &&
                     layers.giveLayer(k).mAlphaLayer[dIP] != byte.MinValue)
                  {
                     float amt = vec[T++];
                     int kQ = layers.giveLayer(k).mAlphaLayer[dIP];
                     int Del = (int)(((hVal * amt) + remainder));
                     int R = kQ - Del;

                     if (R < 0)
                     {
                        remainder = Del - kQ;
                        layers.giveLayer(k).mAlphaLayer[dIP] = 0;
                     }
                     else
                     {
                        layers.giveLayer(k).mAlphaLayer[dIP] = (byte)BMathLib.Clamp(R, 0, 255);
                        remainder = 0;
                     }
                  }
               }
               tLayer.mAlphaLayer[dIP] = (byte)(BMathLib.Clamp(tLayer.mAlphaLayer[dIP] + remainder + val, 0, 255));

            }
         }
      }
      //----------------------------------------------
      unsafe static public bool setIndexToMaskedArea(BTerrainQuadNode node, int minxvert, int minzvert, int maxxvert, int maxzvert, char index)
      {
         bool changed = false;
         uint dstImgWidth = BTerrainTexturing.getAlphaTextureWidth();
         uint dstImgHeight = BTerrainTexturing.getAlphaTextureWidth();
         float vertsToPixelsRatio = BTerrainTexturing.getAlphaTextureWidth() / (float)BTerrainQuadNode.getMaxNodeWidth();

         BTerrainLayerContainer layers = node.mLayerContainer;
         int layerIndex = layers.giveLayerIndex(index, BTerrainTexturingLayer.eLayerType.cLayer_Splat);

         if (TerrainGlobals.getEditor().mEraseTextureInstead && layerIndex == 0)
            return false;

         //layers.removeRedundantLayers();//CLM is this still needed?!??!

         if (layerIndex == -1)
         {
            if (TerrainGlobals.getEditor().mEraseTextureInstead)  //we're erasing, and the texture doesn't exist..
               return false;

            //i don't exist yet.
            newSplatLayerEverywhere(index);
            layerIndex = layers.giveLayerIndex(index, BTerrainTexturingLayer.eLayerType.cLayer_Splat);
            BTerrainTexturingLayer tLayer = layers.giveLayer(layerIndex);

            for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
            {
               for (int z = 0; z < BTerrainTexturing.getAlphaTextureWidth(); z++)
               {
                  //find the closest vert
                  int cX = (int)((x * vertsToPixelsRatio) + minxvert);
                  int cZ = (int)((z * vertsToPixelsRatio) + minzvert);

                  int vertIndex = (int)(cX * TerrainGlobals.getTerrain().getNumZVerts() + cZ);

                  float curWeight = 1.0f;
                  if (Masking.isPointSelected(vertIndex, ref curWeight))
                  {
                     curWeight = BMathLib.Clamp(curWeight, 0, 1);
                     

                     int dIP = x + ((int)BTerrainTexturing.getAlphaTextureHeight()) * z;

                     byte val = (byte)(curWeight * byte.MaxValue);
                     tLayer.mAlphaLayer[dIP] = val;
                  }
               }
            }
            changed = true;
         }
         else
         {
            BTerrainTexturingLayer tLayer = layers.giveLayer(layerIndex);
            //this layer already exists.
            //If a pixel exists above me, subtract me from it, rather than adding to me.
            for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
            {
               for (int z = 0; z < BTerrainTexturing.getAlphaTextureWidth(); z++)
               {
                  //find the closest vert
                  int cX = (int)((x * vertsToPixelsRatio) + minxvert);
                  int cZ = (int)((z * vertsToPixelsRatio) + minzvert);

                  int vertIndex = (int)(cX * TerrainGlobals.getTerrain().getNumZVerts() + cZ);

                  float curWeight = 1.0f;
                  if (Masking.isPointSelected(vertIndex, ref curWeight))
                  {
                     changed = true;
                     int dIP = x + ((int)BTerrainTexturing.getAlphaTextureHeight()) * z;
                     curWeight = BMathLib.Clamp(curWeight, 0, 1);
                     

                     byte val = (byte)(curWeight * byte.MaxValue);

                     valToLayerColumn(layers, tLayer, dIP, val, layerIndex);
                  }
               }
            }
         }

         layers.removeBlankLayers();

         return changed;
      }

      //----------------------------------------------


      unsafe static public byte giveBrushAlphaValue(UInt32* mskImg, uint mskImgWidth, uint mskImgHeight,
                                                    int minxvert, int minzvert, int maxxvert, int maxzvert,
                                                    int terrainGridX, int terrainGridZ,
                                                    int terrainBrushCenterGridX, int terrainBrushCenterGridZ)
      {

         uint maskHalfWidth = mskImgWidth >> 1;
         uint maskHalfHeight = mskImgHeight >> 1;

         int terrainXDelta = terrainGridX - terrainBrushCenterGridX;
         int terrainZDelta = terrainGridZ - terrainBrushCenterGridZ;

         int maskXPos = (int)(maskHalfWidth + terrainXDelta);
         int maskZPos = (int)(maskHalfHeight + terrainZDelta);

         if (maskXPos < 0 || maskXPos >= mskImgWidth || maskZPos < 0 || maskZPos >= mskImgHeight)
            return 0;


         int mIP = maskXPos * ((int)mskImgWidth) + maskZPos;
         byte alpha = (byte)((mskImg[mIP] & 0xFF000000) >> 24);
         return alpha;

      }
      unsafe static public bool setMaskAlphaToLayer(BTerrainQuadNode node, UInt32* mskImg, uint mskImgWidth, uint mskImgHeight, float alphaScalar,
                                                            int terrainGridX, int terrainGridZ, char index)
      {
         bool changed = false;
         uint dstImgWidth = BTerrainTexturing.getAlphaTextureWidth();
         uint dstImgHeight = BTerrainTexturing.getAlphaTextureWidth();
         float vertsToPixelsRatio = BTerrainTexturing.getAlphaTextureWidth() / (float)BTerrainQuadNode.getMaxNodeWidth();


         int minxvert = node.getDesc().mMinXVert;
         int minzvert = node.getDesc().mMinZVert;
         int maxxvert = node.getDesc().mMaxXVert;
         int maxzvert = node.getDesc().mMaxZVert;

         BTerrainLayerContainer layers = node.mLayerContainer;
         int layerIndex = layers.giveLayerIndex(index, BTerrainTexturingLayer.eLayerType.cLayer_Splat);

         if (TerrainGlobals.getEditor().mEraseTextureInstead && layerIndex == 0)
            return false;

         //  layers.removeRedundantLayers();//CLM is this still needed?!??!

         if (layerIndex == -1)
         {
            if (TerrainGlobals.getEditor().mEraseTextureInstead)  //we're erasing, and the texture doesn't exist..
               return false;

            //i don't exist yet.
            newSplatLayerEverywhere(index);
            layerIndex = layers.giveLayerIndex(index, BTerrainTexturingLayer.eLayerType.cLayer_Splat);
            BTerrainTexturingLayer tLayer = layers.giveLayer(layerIndex);

            for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
            {
               for (int z = 0; z < BTerrainTexturing.getAlphaTextureWidth(); z++)
               {
                  float curWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(terrainGridX, terrainGridZ);
                  if (!BMathLib.compare(curWeight, 0.0f))
                  {
                     int cX = (int)((x * vertsToPixelsRatio) + minxvert);
                     int cZ = (int)((z * vertsToPixelsRatio) + minzvert);

                     curWeight = BMathLib.Clamp(curWeight, 0, 1);


                     int dIP = x + ((int)BTerrainTexturing.getAlphaTextureHeight()) * z;

                     byte alphaMapVal = giveBrushAlphaValue(mskImg, mskImgWidth, mskImgHeight, minxvert, minzvert, maxxvert, maxzvert, cX, cZ, terrainGridX, terrainGridZ);

                     byte val = (byte)(alphaMapVal * alphaScalar * curWeight);
                     tLayer.mAlphaLayer[dIP] = val;
                  }
               }
            }
            changed = true;
         }
         else
         {
            BTerrainTexturingLayer tLayer = layers.giveLayer(layerIndex);
            //this layer already exists.
            //If a pixel exists above me, subtract me from it, rather than adding to me.
            for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
            {
               for (int z = 0; z < BTerrainTexturing.getAlphaTextureWidth(); z++)
               {
                  //find the closest vert
                  int cX = (int)((x * vertsToPixelsRatio) + minxvert);
                  int cZ = (int)((z * vertsToPixelsRatio) + minzvert);

                  int dIP = x + ((int)BTerrainTexturing.getAlphaTextureHeight()) * z;

                  float curWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(terrainGridX, terrainGridZ);
                  if (!BMathLib.compare(curWeight, 0.0f))
                  {
                     changed = true;
                     curWeight = BMathLib.Clamp(curWeight, 0, 1);
 

                     byte alphaMapVal = giveBrushAlphaValue(mskImg, mskImgWidth, mskImgHeight, minxvert, minzvert, maxxvert, maxzvert, cX, cZ, terrainGridX, terrainGridZ);

                     byte val = (byte)(alphaMapVal * alphaScalar * curWeight);

                     valToLayerColumn(layers, tLayer, dIP, val, layerIndex);
                  }
               }
            }
         }

         layers.removeBlankLayers();

         return changed;
      }
      //----------------------------------------------
      unsafe static public bool setMaskAlphaToTextureBlending(BTerrainQuadNode node, uint dstImgWidth, uint dstImgHeight,
                                                            UInt32* mskImg, uint mskImgWidth, uint mskImgHeight, float intensity,
                                                            int xpos, int ypos, bool alternate)
      {
         BTerrainQuadNodeDesc desc = node.getDesc();

         bool changed = false;

         //create our position in the dst img, adjust for boundries
         int mskMaxX = (int)mskImgWidth;
         if (mskImgWidth + xpos >= dstImgWidth) mskMaxX -= ((int)mskImgWidth) + xpos - ((int)dstImgWidth);

         int mskMaxY = (int)mskImgHeight;
         if (mskImgHeight + ypos >= dstImgHeight) mskMaxY -= ((int)mskImgHeight) + ypos - ((int)dstImgHeight);

         int mskMinX = 0;
         int mskMinY = 0;
         if (xpos < 0) mskMinX = -xpos;
         if (ypos < 0) mskMinY = -ypos;

         //validate extents...
         if (mskMinX > mskImgWidth || mskMinY > mskImgHeight)
            return false;


         int minX = (int)(desc.m_min.X / TerrainGlobals.getTerrain().getTileScale());
         int maxX = (int)(desc.m_max.X / TerrainGlobals.getTerrain().getTileScale());
         int minZ = (int)(desc.m_min.Z / TerrainGlobals.getTerrain().getTileScale());
         int maxZ = (int)(desc.m_max.Z / TerrainGlobals.getTerrain().getTileScale());

         for (int x = minX; x <= maxX; x++)
         {
            for (int z = minZ; z <= maxZ; z++)
            {
               int maskX = x - minX + mskMinX;
               int maskY = z - minZ + mskMinY;

               int mIP = maskX * ((int)mskImgWidth) + maskY;
               char alpha = (char)(((mskImg[mIP] & 0xFF000000) >> 24));

               float factor = alpha / 255.0f * intensity; ;

               int vertIndex = (int)(x * TerrainGlobals.getTerrain().getNumZVerts() + z);
               float curWeight = Masking.getCurrSelectionMaskWeights().GetMaskWeight(vertIndex);
               float newWeight;

               if (!alternate)
                  newWeight = (curWeight > factor) ? curWeight : factor;
               else//CLM Changes to erasing..
                  newWeight = BMathLib.Clamp(curWeight - factor, 0, 1);
               Masking.addSelectedVert(x, z, newWeight);
               changed = true;

            }
         }

         return changed;
      }

      //----------------------------------------------
      //----------------------------------------------


   }
}