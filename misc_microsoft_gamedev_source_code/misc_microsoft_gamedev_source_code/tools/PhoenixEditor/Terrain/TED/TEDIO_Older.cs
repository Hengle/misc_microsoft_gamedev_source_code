using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Windows.Forms;
using System.Threading;
using Xceed.Zip;
using Xceed.FileSystem;
//
using EditorCore;
using Rendering;
using SimEditor;
using Export360;

namespace Terrain
{

   public partial class TEDIO
   {
      //----------------------------------------------
      #region RESIZE TED DATA v25
      static public void resamplePreV25Map(ref TEDLoadedData dat, ref TerrainCreationParams param)
      {

         //CLM what about non POW2 maps?!?!
         int newSizeX = param.mNumVisXVerts >> 1;
         int newSizeZ = param.mNumVisZVerts >> 1;



         resamplePreV25VertexData(ref dat, ref param, newSizeX, newSizeZ);
         GC.WaitForPendingFinalizers();
         GC.Collect();

         resamplePreV25TextureData(ref dat, ref param, newSizeX, newSizeZ);
         GC.WaitForPendingFinalizers();
         GC.Collect();

         param.initFromVisData(newSizeX, newSizeZ, TerrainCreationParams.cTileSpacingOne, 8);

         resamplePreV26Map(ref dat, ref param);
      }

      static void resamplePreV25VertexData(ref TEDLoadedData dat, ref TerrainCreationParams param, int newSizeX, int newSizeZ)
      {
         ImageManipulation.eFilterType FilterType = ImageManipulation.eFilterType.cFilter_Linear;

         //XYZ DISPLACEMENT
         float[] tDatX = new float[param.mNumVisXVerts * param.mNumVisZVerts];
         float[] tDatY = new float[param.mNumVisXVerts * param.mNumVisZVerts];
         float[] tDatZ = new float[param.mNumVisXVerts * param.mNumVisZVerts];

         for (int i = 0; i < param.mNumVisXVerts * param.mNumVisZVerts; i++)
         {
            tDatX[i] = dat.mPositions[i].X;
            tDatY[i] = dat.mPositions[i].Y;
            tDatZ[i] = dat.mPositions[i].Z;
         }

         float[] imgScaledX = ImageManipulation.resizeF32Img(tDatX, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, FilterType);
         float[] imgScaledY = ImageManipulation.resizeF32Img(tDatY, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, FilterType);
         float[] imgScaledZ = ImageManipulation.resizeF32Img(tDatZ, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, FilterType);

         dat.mPositions = null;
         dat.mPositions = new Vector3[newSizeX * newSizeZ];
         for (int i = 0; i < imgScaledX.Length; i++)
         {
            dat.mPositions[i].X = imgScaledX[i];
            dat.mPositions[i].Y = imgScaledY[i];
            dat.mPositions[i].Z = imgScaledZ[i];
         }
         tDatX = null;
         tDatY = null;
         tDatZ = null;
         imgScaledX = null;
         imgScaledY = null;
         imgScaledZ = null;


         //ALPHA VALUES
         dat.mAlphaValues = ImageManipulation.resizeGreyScaleImg(dat.mAlphaValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, FilterType);

         dat.mSimHeightPassValues.SetEmptyValue(0);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         dat.mSimHeightBuildValues.SetEmptyValue(0);
         dat.mCameraHeightOverrideValues.SetEmptyValue(CameraHeightRep.cJaggedEmptyValue);
         dat.mFlightHeightOverrideValues.SetEmptyValue(CameraHeightRep.cJaggedEmptyValue);

         resampleJaggedArrayInt(ref dat.mSimHeightPassValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, 0, false);
         resampleJaggedArrayFloat(ref dat.mSimHeightOverrideValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, SimHeightRep.cJaggedEmptyValue, false);
         resampleJaggedArrayByte(ref dat.mTessellationOverrideValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, BTerrainEditor.cTesselationEmptyVal, false);
         resampleJaggedArrayInt(ref dat.mSimHeightBuildValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, 0, false);

         //Camera is a seperate sparce data set
         int oldCameraX = param.mNumVisXVerts >> 6;
         int oldCameraZ = param.mNumVisZVerts >> 6;
         int newCameraX = newSizeX >> 6;
         int newCameraZ = newSizeZ >> 6;
         resampleJaggedArrayFloat(ref dat.mCameraHeightOverrideValues, oldCameraX, oldCameraZ, newCameraX, newCameraZ, CameraHeightRep.cJaggedEmptyValue, false);
         resampleJaggedArrayFloat(ref dat.mFlightHeightOverrideValues, oldCameraX, oldCameraZ, newCameraX, newCameraZ, SimFlightRep.cJaggedEmptyValue, false);

         resampleFoliage(ref dat, ref param, newSizeX, newSizeZ);
         resampleRoads(ref dat, ref param, newSizeX, newSizeZ);
      }
      static void resamplePreV25TextureData(ref TEDLoadedData dat, ref TerrainCreationParams param, int newSizeX, int newSizeZ)
      {

         //walk through all the verts in the terrain. If they have mask data, copy them
         int w = param.mNumVisXVerts;
         int h = param.mNumVisZVerts;

         BTerrainTextureVector[,] mNewTextureData = new BTerrainTextureVector[newSizeX, newSizeZ];
         BTerrainTextureVector layerProto = new BTerrainTextureVector();

         /////////////////////////////////
         // Convert data to vectors
         /////////////////////////////////

         BTerrainTextureVector tv = null;
         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            int xIndex = (int)(dat.mTextureHolders[i].mXIndex * BTerrainQuadNode.cMaxWidth);
            int zIndex = (int)(dat.mTextureHolders[i].mZIndex * BTerrainQuadNode.cMaxHeight);
            for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
            {
               for (int z = 0; z < BTerrainTexturing.getAlphaTextureHeight(); z++)
               {
                  tv = dat.mTextureHolders[i].giveLayerChainAtPixel(x, z);

                  if (tv == null || tv.mLayers.Count == 0)
                     continue;

                  //add any textures to the list that we need.
                  for (int k = 0; k < tv.mLayers.Count; k++)
                  {
                     //don't store decal data ????
                     if (tv.mLayers[k].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
                     {
                        tv.mLayers.RemoveAt(k);
                        k--;
                        continue;
                     }
                  }
                  layerProto.mergeWith(tv, true);
                  tv = null;
               }
            }
         }



         /////////////////////////////////
         //Scale everything
         /////////////////////////////////
         ImageManipulation.eFilterType FilterType = ImageManipulation.eFilterType.cFilter_Linear;

         byte[] extDat = new byte[w * h];
         byte[] imgScaledX = new byte[newSizeX * newSizeZ];

         for (int i = 0; i < layerProto.getNumLayers(); i++)
         {

            for (int x = 0; x < w * h; x++)
               extDat[x] = 0;

            //extract
            for (int j = 0; j < dat.mTextureHolders.Length; j++)
            {
               int xIndex = (int)(dat.mTextureHolders[j].mXIndex * BTerrainQuadNode.cMaxWidth);
               int zIndex = (int)(dat.mTextureHolders[j].mZIndex * BTerrainQuadNode.cMaxHeight);
               if (!dat.mTextureHolders[j].containsID(layerProto.mLayers[i].mActiveTextureIndex, BTerrainTexturingLayer.eLayerType.cLayer_Splat))
                  continue;

               for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
               {
                  for (int z = 0; z < BTerrainTexturing.getAlphaTextureHeight(); z++)
                  {
                     tv = dat.mTextureHolders[j].giveLayerChainAtPixel(x, z);
                     if (tv == null || tv.mLayers == null || tv.mLayers.Count == 0)
                        continue;

                     int idx = tv.giveLayerIndex(layerProto.mLayers[i].mActiveTextureIndex, layerProto.mLayers[i].mLayerType);
                     if (idx != -1)
                     {
                        int dstIndex = (x + xIndex) * w + (z + zIndex);
                        extDat[dstIndex] = tv.mLayers[idx].mAlphaContrib;
                     }
                  }
               }
            }


            //scale
            ImageManipulation.resizeGreyScaleImg(extDat, imgScaledX, w, h, newSizeX, newSizeZ, FilterType);

            //pass 
            for (int k = 0; k < newSizeX; k++)
            {
               for (int j = 0; j < newSizeZ; j++)
               {
                  int ttdIndex = k * newSizeX + j;

                  byte value = imgScaledX[ttdIndex];
                  if (value == 0)
                     continue;

                  BTerrainTextureVector od = mNewTextureData[k, j];
                  if (od == null)
                     od = new BTerrainTextureVector();

                  od.initLayer(i, layerProto.mLayers[i].mActiveTextureIndex, value, layerProto.mLayers[i].mLayerType);
                  mNewTextureData[k, j] = od;
               }
            }
         }

         /////////////////////////////////
         // send back to main data
         /////////////////////////////////
         //OK to clear this?? Or do i still need data from it?
         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i].destroy();
            dat.mTextureHolders[i] = null;
         }
         dat.mTextureHolders = null;
         extDat = null;
         imgScaledX = null;

         GC.WaitForPendingFinalizers();
         GC.Collect();
         int numNewChunksX = (int)(newSizeX / BTerrainQuadNode.cMaxWidth);
         dat.mTextureHolders = new TED_TextureDataQN[numNewChunksX * numNewChunksX];

         for (int i = 0; i < numNewChunksX; i++)
         {
            for (int j = 0; j < numNewChunksX; j++)
            {
               int index = i + numNewChunksX * j;
               dat.mTextureHolders[index] = new TED_TextureDataQN();
               dat.mTextureHolders[index].mXIndex = i;
               dat.mTextureHolders[index].mZIndex = j;
               dat.mTextureHolders[index].mLayers = new List<BTerrainTexturingLayer>();

               int minxVert = (int)(i * BTerrainQuadNode.cMaxWidth);
               int minzVert = (int)(j * BTerrainQuadNode.cMaxHeight);

               for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
               {
                  for (int z = 0; z < BTerrainTexturing.getAlphaTextureHeight(); z++)
                  {
                     BTerrainTextureVector v = mNewTextureData[minxVert + x, minzVert + z];
                     if (v == null || v.getNumLayers() == 0)
                     {
                        //v = new BTerrainTextureVector();
                        //v.initLayer(0, 0, 255, BTerrainTexturingLayer.eLayerType.cLayer_Splat);
                        continue;
                     }

                     int aIndex = (int)(x + z * BTerrainTexturing.getAlphaTextureHeight());

                     for (int k = 0; k < v.getNumLayers(); k++)
                     {
                        if (dat.mTextureHolders[index].containsID(v.mLayers[k].mActiveTextureIndex, v.mLayers[k].mLayerType))
                        {
                           dat.mTextureHolders[index].giveLayer(dat.mTextureHolders[index].giveLayerIndex(v.mLayers[k].mActiveTextureIndex, v.mLayers[k].mLayerType)).mAlphaLayer[aIndex] = v.mLayers[k].mAlphaContrib;
                        }
                        else
                        {
                           int lIdx = 0;
                           if (v.mLayers[k].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
                              lIdx = dat.mTextureHolders[index].newSplatLayer(v.mLayers[k].mActiveTextureIndex);
                           else
                              lIdx = dat.mTextureHolders[index].newDecalLayer(v.mLayers[k].mActiveTextureIndex);

                           dat.mTextureHolders[index].giveLayer(lIdx).mAlphaLayer[aIndex] = v.mLayers[k].mAlphaContrib;
                        }
                     }
                  }
               }
            }
         }

         mNewTextureData = null;

      }

      #endregion
      #region RESIZE TED DATA v26
      static public void resamplePreV26Map(ref TEDLoadedData dat, ref TerrainCreationParams param)
      {

         //CLM v26 changed sim data to be hardcoded



         //heights data
         int newSizeX = param.mNumSimXTiles + 1;
         int newSizeZ = param.mNumSimXTiles + 1;
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         resampleJaggedArrayFloat(ref dat.mSimHeightOverrideValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, SimHeightRep.cJaggedEmptyValue, false);


         //tile data
         newSizeX = param.mNumSimXTiles;
         newSizeZ = param.mNumSimXTiles;
         dat.mSimHeightPassValues.SetEmptyValue(0);
         dat.mSimHeightBuildValues.SetEmptyValue(0);
         resampleJaggedArrayInt(ref dat.mSimHeightPassValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, 0, true);
         resampleJaggedArrayInt(ref dat.mSimHeightBuildValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, 0, true);


         GC.WaitForPendingFinalizers();
         GC.Collect();


      }

      #endregion
      //----------------------------------------------
      static public void LoadPreV7Texturing(byte[] v6IndexTexture, byte[] v6AlphaTexture, ref TED_TextureDataQN texDat)
      {
         texDat.mLayers = new List<BTerrainTexturingLayer>();
         BTerrainTexturingLayer lyr = new BTerrainTexturingLayer();
         lyr.mLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
         lyr.mActiveTextureIndex = 0;
         lyr.mAlphaLayer = new byte[BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight()];
         for (int k = 0; k < BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight(); k++)
            lyr.mAlphaLayer[k] = 255;

         texDat.mLayers.Add(lyr);

      }
      //----------------------------------------------
      static public void LoadTEDv4AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {

         int numXVerts = 0;
         int numZVerts = 0;
         int numVerts = 0;
         float tileScale = 0;
         {
            // read header
            int numXTiles = f.ReadInt32();
            int numZTiles = f.ReadInt32();
            tileScale = f.ReadSingle();

            param.mNumVisXVerts = numXTiles + 1;
            param.mNumVisZVerts = numZTiles + 1;
            param.mVisTileSpacing = 0.25f;

            param.mSimTileSpacing = 1;
            param.mNumSimXTiles = (int)(numXTiles * 0.25f);
            param.mNumSimZTiles = (int)(numXTiles * 0.25f);
            param.mSimToVisMultiplier = 4;
         }


         numVerts = param.mNumVisXVerts * param.mNumVisZVerts;


         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.SettingsQuick();

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         if (versionNumber >= 2)
         {
            // Read grip points
            uint numGridPoints = f.ReadUInt32();
            float t = 0;
            for (int i = 0; i < numGridPoints; i++)
            {
               t = f.ReadSingle();
               t = f.ReadSingle();
               t = f.ReadSingle();
            }
         }
         else
         {
            float t = 0;
            for (int i = 0; i < numVerts; i++)
            {
               t = f.ReadSingle();
            }

         }

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = 255;

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         TerrainGlobals.getTerrain().createFromTED(param, dat.mPositions, dat.mAlphaValues, dat.mAOValues, dat.mSimHeightBuildValues, dat.mSimHeightPassValues, dat.mSimHeightOverrideValues, dat.mTessellationOverrideValues, dat.mCameraHeightOverrideValues, dat.mFlightHeightOverrideValues,dat.mSimFloodPassValues,dat.mSimScarabPassValues,dat.mSimTileTypeValues);

         // read out texture information
         BTerrainQuadNode[] nodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();


         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;


         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            BTerrainQuadNodeDesc desc = nodes[i].getDesc();
            int index = (desc.mMinXVert / param.mNumVisXVerts) + param.mNumVisXVerts * (desc.mMinZVert / param.mNumVisXVerts);

            dat.mTextureHolders[i].mXIndex = (int)(desc.mMinXVert / BTerrainQuadNode.getMaxNodeWidth());
            dat.mTextureHolders[i].mZIndex = (int)(desc.mMinZVert / BTerrainQuadNode.getMaxNodeWidth());

            byte[] v6IndexTextures = new byte[indexTexHeight * indexTexPitch];
            f.Read(v6IndexTextures, 0, (int)(indexTexHeight * indexTexPitch));

            byte[] v6AlphaTextures = new byte[indexTexHeight * indexTexPitch];
            f.Read(v6AlphaTextures, 0, (int)(alphaTexHeight * alphaTexPitch));

            LoadPreV7Texturing(v6IndexTextures, v6AlphaTextures, ref dat.mTextureHolders[i]);
         }


         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();
         if (versionNumber == 3)
         {
            dat.mActiveTextures = new TED_ActiveTextureData[16];
            for (int i = 0; i < 16; i++)
            {
               dat.mActiveTextures[i] = new TED_ActiveTextureData();
               dat.mActiveTextures[i].mFilename = f.ReadString();
               /*
               //textureNames[i] = f.ReadString();
               string texfile = CoreGlobals.getWorkPaths().ConverToLocalWorkPath(f.ReadString());
               texfile=Path.ChangeExtension(texfile, ".ddx");
               if(File.Exists(texfile))
               {

                  SimTerrainType.mActiveWorkingSet.Add(SimTerrainType.getIndexFromDef(SimTerrainType.getFromTextureName(texfile)));
               }
               else
               {
               }*/
            }
         }
         else if (versionNumber == 4)
         {
            dat.mActiveTextures = new TED_ActiveTextureData[16];
            {
               for (int i = 0; i < 16; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
               }
            }
         }

         dat.mActiveDecals = null;
         dat.mActiveDecalInstances = null;


         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv5AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {

         // read version number         
         if (versionNumber < 5)
         {
            LoadTEDv4AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         // TDHeader head = new TDHeader();
         //head.Read(f);
         int numSimXTiles = f.ReadInt32();
         int numSimZTiles = f.ReadInt32();
         float simTileSize = f.ReadSingle();
         int simToVisMultiplier = f.ReadInt32();

         param.initFromSimData(numSimXTiles, numSimZTiles, simTileSize, simToVisMultiplier);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.SettingsQuick();

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = 255;

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         TerrainGlobals.getTerrain().createFromTED(param, dat.mPositions, dat.mAlphaValues, dat.mAOValues, dat.mSimHeightBuildValues, dat.mSimHeightPassValues, dat.mSimHeightOverrideValues, dat.mTessellationOverrideValues, dat.mCameraHeightOverrideValues,dat.mFlightHeightOverrideValues, dat.mSimFloodPassValues,dat.mSimScarabPassValues,dat.mSimTileTypeValues);

         BTerrainQuadNode[] nodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            BTerrainQuadNodeDesc desc = nodes[i].getDesc();
            dat.mTextureHolders[i].mXIndex = (int)(desc.mMinXVert / BTerrainQuadNode.getMaxNodeWidth());
            dat.mTextureHolders[i].mZIndex = (int)(desc.mMinZVert / BTerrainQuadNode.getMaxNodeWidth());

            byte[] v6IndexTextures = new byte[indexTexHeight * indexTexPitch];
            f.Read(v6IndexTextures, 0, (int)(indexTexHeight * indexTexPitch));

            byte[] v6AlphaTextures = new byte[indexTexHeight * indexTexPitch];
            f.Read(v6AlphaTextures, 0, (int)(alphaTexHeight * alphaTexPitch));

            LoadPreV7Texturing(v6IndexTextures, v6AlphaTextures, ref dat.mTextureHolders[i]);
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();
         dat.mActiveTextures = new TED_ActiveTextureData[16];
         {
            for (int i = 0; i < 16; i++)
            {
               dat.mActiveTextures[i] = new TED_ActiveTextureData();
               dat.mActiveTextures[i].mFilename = f.ReadString();
            }
         }

         dat.mActiveDecals = null;
         dat.mActiveDecalInstances = null;

         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv6AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         // read version number         
         if (versionNumber < 6)
         {
            LoadTEDv5AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         // TDHeader head = new TDHeader();
         //head.Read(f);
         int numSimXTiles = f.ReadInt32();
         int numSimZTiles = f.ReadInt32();
         float simTileSize = f.ReadSingle();
         int simToVisMultiplier = f.ReadInt32();

         param.initFromSimData(numSimXTiles, numSimZTiles, simTileSize, simToVisMultiplier);
         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;

         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.SettingsQuick();

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = 255;

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();

            byte[] v6IndexTextures = new byte[indexTexHeight * indexTexPitch];
            f.Read(v6IndexTextures, 0, (int)(indexTexHeight * indexTexPitch));

            byte[] v6AlphaTextures = new byte[indexTexHeight * indexTexPitch];
            f.Read(v6AlphaTextures, 0, (int)(alphaTexHeight * alphaTexPitch));

            LoadPreV7Texturing(v6IndexTextures, v6AlphaTextures, ref dat.mTextureHolders[i]);
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();
         dat.mActiveTextures = new TED_ActiveTextureData[16];
         {
            for (int i = 0; i < 16; i++)
            {
               dat.mActiveTextures[i] = new TED_ActiveTextureData();
               dat.mActiveTextures[i].mFilename = f.ReadString();
            }
         }
         dat.mActiveDecals = null;
         dat.mActiveDecalInstances = null;


         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv7AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         // read version number         
         if (versionNumber < 7)
         {
            LoadTEDv6AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         // TDHeader head = new TDHeader();
         //head.Read(f);
         int numSimXTiles = f.ReadInt32();
         int numSimZTiles = f.ReadInt32();
         float simTileSize = f.ReadSingle();
         int simToVisMultiplier = f.ReadInt32();

         param.initFromSimData(numSimXTiles, numSimZTiles, simTileSize, simToVisMultiplier);
         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = 3;
         dat.mExportSettings.ObjectsInAO = false;


         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = 255;

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();
         int numActiveSet = f.ReadInt32();
         dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
         {
            for (int i = 0; i < numActiveSet; i++)
            {
               dat.mActiveTextures[i] = new TED_ActiveTextureData();
               dat.mActiveTextures[i].mFilename = f.ReadString();
            }
         }
         dat.mActiveDecals = null;
         dat.mActiveDecalInstances = null;

         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv8AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         // read version number         
         if (versionNumber < 8)
         {
            LoadTEDv7AndLater(f, versionNumber, ref dat, ref param);
            return;
         }
         // read header
         // TDHeader head = new TDHeader();
         //head.Read(f);
         int numSimXTiles = f.ReadInt32();
         int numSimZTiles = f.ReadInt32();
         float simTileSize = f.ReadSingle();
         int simToVisMultiplier = f.ReadInt32();

         param.initFromSimData(numSimXTiles, numSimZTiles, simTileSize, simToVisMultiplier);
         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = 3;
         dat.mExportSettings.ObjectsInAO = false;


         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = 255;

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         int numActiveSet = f.ReadInt32();
         dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
         {
            for (int i = 0; i < numActiveSet; i++)
            {
               dat.mActiveTextures[i] = new TED_ActiveTextureData();
               dat.mActiveTextures[i].mFilename = f.ReadString();
               dat.mActiveTextures[i].mUScale = f.ReadInt32();
               dat.mActiveTextures[i].mVScale = f.ReadInt32();
               dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
            }
         }
         dat.mActiveDecalInstances = null;
         dat.mActiveDecals = null;

         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv9AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         // read version number         
         if (versionNumber < 9)
         {
            LoadTEDv8AndLater(f, versionNumber, ref dat, ref param);
            return;
         }
         // read header
         // TDHeader head = new TDHeader();
         //head.Read(f);
         int numSimXTiles = f.ReadInt32();
         int numSimZTiles = f.ReadInt32();
         float simTileSize = f.ReadSingle();
         int simToVisMultiplier = f.ReadInt32();

         param.initFromSimData(numSimXTiles, numSimZTiles, simTileSize, simToVisMultiplier);
         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = 3;
         dat.mExportSettings.ObjectsInAO = false;

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = 255;

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         int numActiveSet = f.ReadInt32();
         dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
         {
            for (int i = 0; i < numActiveSet; i++)
            {
               dat.mActiveTextures[i] = new TED_ActiveTextureData();
               dat.mActiveTextures[i].mFilename = f.ReadString();
               dat.mActiveTextures[i].mUScale = f.ReadInt32();
               dat.mActiveTextures[i].mVScale = f.ReadInt32();
               dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
            }
         }

         // Read Decal Names
         TerrainGlobals.getTexturing().destroyActiveDecals();
         int numActiveDecals = f.ReadInt32();
         if (numActiveDecals != 0)
         {
            dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
            for (int i = 0; i < numActiveDecals; i++)
            {
               dat.mActiveDecals[i] = new TED_ActiveDecalData();
               dat.mActiveDecals[i].mFilename = f.ReadString();
            }
         }



         // Read Decal Instances
         int numActiveDecalInstances = f.ReadInt32();
         dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
         if (numActiveDecalInstances != 0)
         {
            for (int i = 0; i < numActiveDecalInstances; i++)
            {
               dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
               dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
               dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
               dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
               dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
               dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
               dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
               dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
               dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
               dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
               dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
            }
         }

         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv10AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         // read version number         
         if (versionNumber < 10)
         {
            LoadTEDv9AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         // TDHeader head = new TDHeader();
         //head.Read(f);
         int numSimXTiles = f.ReadInt32();
         int numSimZTiles = f.ReadInt32();
         float simTileSize = f.ReadSingle();
         int simToVisMultiplier = f.ReadInt32();

         param.initFromSimData(numSimXTiles, numSimZTiles, simTileSize, simToVisMultiplier);
         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = 3;
         dat.mExportSettings.ObjectsInAO = false;

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = 255;


         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv11AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 11)
         {
            LoadTEDv10AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         // TDHeader head = new TDHeader();
         //head.Read(f);
         int numSimXTiles = f.ReadInt32();
         int numSimZTiles = f.ReadInt32();
         float simTileSize = f.ReadSingle();
         int simToVisMultiplier = f.ReadInt32();

         param.initFromSimData(numSimXTiles, numSimZTiles, simTileSize, simToVisMultiplier);
         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = 3;
         dat.mExportSettings.ObjectsInAO = false;

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);


         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts); 
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv12AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 12)
         {
            LoadTEDv11AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         // TDHeader head = new TDHeader();
         //head.Read(f);
         int numSimXTiles = f.ReadInt32();
         int numSimZTiles = f.ReadInt32();
         float simTileSize = f.ReadSingle();
         int simToVisMultiplier = f.ReadInt32();

         param.initFromSimData(numSimXTiles, numSimZTiles, simTileSize, simToVisMultiplier);
         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = 3;
         dat.mExportSettings.ObjectsInAO = false;

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv13AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 13)
         {
            LoadTEDv12AndLater(f, versionNumber, ref dat, ref param);
            return;
         }


         // read header
         // TDHeader head = new TDHeader();
         //head.Read(f);
         int numSimXTiles = f.ReadInt32();
         int numSimZTiles = f.ReadInt32();
         float simTileSize = f.ReadSingle();
         int simToVisMultiplier = f.ReadInt32();

         param.initFromSimData(numSimXTiles, numSimZTiles, simTileSize, simToVisMultiplier);
         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = false;

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv14AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 14)
         {
            LoadTEDv13AndLater(f, versionNumber, ref dat, ref param);
            return;
         }


         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = false;

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv15AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 15)
         {
            LoadTEDv14AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = false;

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);


         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         for (int i = 0; i < numVerts; i++)
         {
            bool k = f.ReadBoolean();
            if (!k)
               dat.mSimHeightPassValues.SetValue(i, -1);
         }


         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }
      }
      //----------------------------------------------
      static public void LoadTEDv16AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 16)
         {
            LoadTEDv15AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = false;

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         for (int i = 0; i < numVerts; i++)
         {
            bool k = f.ReadBoolean();
            if (!k)
               dat.mSimHeightPassValues.SetValue(i, -1);
         }
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }

         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv17AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 17)
         {
            LoadTEDv16AndLater(f, versionNumber, ref dat, ref param);
            return;
         }
         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = false;

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         for (int i = 0; i < numVerts; i++)
         {
            bool k = f.ReadBoolean();
            if (!k)
               dat.mSimHeightPassValues.SetValue(i, -1);
         }

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         for (int i = 0; i < numVerts; i++)
         {
            int idx = f.ReadInt32();
            float v = f.ReadSingle();
            if (v != SimHeightRep.cJaggedEmptyValue)
            {
               dat.mSimHeightOverrideValues.SetValue(idx, v);
            }
         }

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }

         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv18AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 18)
         {
            LoadTEDv17AndLater(f, versionNumber, ref dat, ref param);
            return;
         }
         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = false;

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         for (int i = 0; i < numVerts; i++)
         {
            bool k = f.ReadBoolean();
            if (!k)
               dat.mSimHeightPassValues.SetValue(i, -1);
         }
         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         for (int i = 0; i < numVerts; i++)
         {
            int idx = f.ReadInt32();
            float v = f.ReadSingle();
            if (v != SimHeightRep.cJaggedEmptyValue)
            {
               dat.mSimHeightOverrideValues.SetValue(idx, v);
            }
         }

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         for (int i = 0; i < numVerts; i++)
         {
            int idx = f.ReadInt32();
            byte v = f.ReadByte();
            if (v != BTerrainEditor.cTesselationEmptyVal)
            {
               dat.mTessellationOverrideValues.SetValue(idx, v);
            }
         }
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);


         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }

         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv19AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 19)
         {
            LoadTEDv18AndLater(f, versionNumber, ref dat, ref param);
            return;
         }
         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         for (int i = 0; i < numVerts; i++)
         {
            bool k = f.ReadBoolean();
            if (!k)
               dat.mSimHeightPassValues.SetValue(i, -1);
         }

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         for (int i = 0; i < numVerts; i++)
         {
            int idx = f.ReadInt32();
            float v = f.ReadSingle();
            if (v != SimHeightRep.cJaggedEmptyValue)
            {
               dat.mSimHeightOverrideValues.SetValue(idx, v);
            }
         }

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         for (int i = 0; i < numVerts; i++)
         {
            int idx = f.ReadInt32();
            byte v = f.ReadByte();
            if (v != BTerrainEditor.cTesselationEmptyVal)
            {
               dat.mTessellationOverrideValues.SetValue(idx, v);
            }
         }
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }

         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv20AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 20)
         {
            LoadTEDv19AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         for (int i = 0; i < numVerts; i++)
         {
            bool k = f.ReadBoolean();
            if (!k)
               dat.mSimHeightPassValues.SetValue(i, -1);
         }

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         for (int i = 0; i < numVerts; i++)
         {
            int idx = f.ReadInt32();
            float v = f.ReadSingle();
            if (v != SimHeightRep.cJaggedEmptyValue)
            {
               dat.mSimHeightOverrideValues.SetValue(idx, v);
            }
         }

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         for (int i = 0; i < numVerts; i++)
         {
            int idx = f.ReadInt32();
            byte v = f.ReadByte();
            if (v != BTerrainEditor.cTesselationEmptyVal)
            {
               dat.mTessellationOverrideValues.SetValue(idx, v);
            }
         }

         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }


         //FOLIAGE
         int numFoliageSets = f.ReadInt32();
         if (numFoliageSets > 0)
         {
            dat.mFoliageData = new TED_FoliageData();
            dat.mFoliageData.mSetFileNames = new List<string>();
            for (int i = 0; i < numFoliageSets; i++)
            {
               dat.mFoliageData.mSetFileNames.Add(f.ReadString());
            }

            dat.mFoliageData.mFoliageVerts = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(numVerts);
            for (int i = 0; i < numVerts; i++)
            {
               int idx = f.ReadInt32();
               TED_FoliageData.TED_FoliageVertData v = new TED_FoliageData.TED_FoliageVertData();
               v.mFoliageSetIndex = f.ReadInt32();
               v.mFoliageSetBladeIndex = f.ReadInt32();
               if (v.mFoliageSetIndex != -1)
               {
                  dat.mFoliageData.mFoliageVerts.SetValue(idx, v);
               }
            }
         }

         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv21AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 21)
         {
            LoadTEDv20AndLater(f, versionNumber, ref dat, ref param);
            return;
         }


         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightPassValues.SetEmptyValue(0);
         for (int i = 0; i < numVerts; i++)
         {
            int idx = f.ReadInt32();
            int v = f.ReadInt32();
            if (v != 0)
            {
               dat.mSimHeightPassValues.SetValue(idx, v);
            }
         }

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         for (int i = 0; i < numVerts; i++)
         {
            int idx = f.ReadInt32();
            float v = f.ReadSingle();
            if (v != SimHeightRep.cJaggedEmptyValue)
            {
               dat.mSimHeightOverrideValues.SetValue(idx, v);
            }
         }

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         for (int i = 0; i < numVerts; i++)
         {
            int idx = f.ReadInt32();
            byte v = f.ReadByte();
            if (v != BTerrainEditor.cTesselationEmptyVal)
            {
               dat.mTessellationOverrideValues.SetValue(idx, v);
            }
         }
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }


         //FOLIAGE
         int numFoliageSets = f.ReadInt32();
         if (numFoliageSets > 0)
         {
            dat.mFoliageData = new TED_FoliageData();
            dat.mFoliageData.mSetFileNames = new List<string>();
            for (int i = 0; i < numFoliageSets; i++)
            {
               dat.mFoliageData.mSetFileNames.Add(f.ReadString());
            }

            dat.mFoliageData.mFoliageVerts = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(numVerts);
            for (int i = 0; i < numVerts; i++)
            {
               int idx = f.ReadInt32();
               TED_FoliageData.TED_FoliageVertData v = new TED_FoliageData.TED_FoliageVertData();
               v.mFoliageSetIndex = f.ReadInt32();
               v.mFoliageSetBladeIndex = f.ReadInt32();
               if (v.mFoliageSetIndex != -1)
               {
                  dat.mFoliageData.mFoliageVerts.SetValue(idx, v);
               }
            }
         }

         f.Close();
         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv22AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 22)
         {
            LoadTEDv21AndLater(f, versionNumber, ref dat, ref param);
            return;
         }



         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightPassValues.SetEmptyValue(0);
         //for (int i = 0; i < numVerts; i++)
         //{
         //   int idx = f.ReadInt32();
         //   int v = f.ReadInt32();
         //   if (v != 0)
         //   {
         //      dat.mSimHeightPassValues.SetValue(idx, v);
         //   }
         //}
         dat.mSimHeightPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         //for (int i = 0; i < numVerts; i++)
         //{
         //   int idx = f.ReadInt32();
         //   float v = f.ReadSingle();
         //   if (v != SimHeightRep.cJaggedEmptyValue)
         //   {
         //      dat.mSimHeightOverrideValues.SetValue(idx, v);
         //   }
         //}
         dat.mSimHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         //for (int i = 0; i < numVerts; i++)
         //{
         //   int idx = f.ReadInt32();
         //   byte v = f.ReadByte();
         //   if (v != BTerrainEditor.cTesselationEmptyVal)
         //   {
         //      dat.mTessellationOverrideValues.SetValue(idx, v);
         //   }
         //}
         dat.mTessellationOverrideValues.LoadByStripe(f,
            (JaggedContainer<byte>.LoadStripeDelegate)(delegate(BinaryReader r, byte[] values)
            {
               r.Read(values, 0, values.Length);
            }));

         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }


         //FOLIAGE
         int numFoliageSets = f.ReadInt32();
         if (numFoliageSets > 0)
         {
            dat.mFoliageData = new TED_FoliageData();
            dat.mFoliageData.mSetFileNames = new List<string>();
            for (int i = 0; i < numFoliageSets; i++)
            {
               dat.mFoliageData.mSetFileNames.Add(f.ReadString());
            }

            dat.mFoliageData.mFoliageVerts = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(numVerts);
            //for (int i = 0; i < numVerts; i++)
            //{
            //   int idx = f.ReadInt32();
            //   TED_FoliageData.TED_FoliageVertData v = new TED_FoliageData.TED_FoliageVertData();
            //   v.mFoliageSetIndex = f.ReadInt32();
            //   v.mFoliageSetBladeIndex = f.ReadInt32();
            //   if (v.mFoliageSetIndex != -1)
            //   {
            //      dat.mFoliageData.mFoliageVerts.SetValue(idx, v);
            //   }
            //}
            dat.mFoliageData.mFoliageVerts.LoadByStripe(f,
               (JaggedContainer<TED_FoliageData.TED_FoliageVertData>.LoadStripeDelegate)(delegate(BinaryReader r, TED_FoliageData.TED_FoliageVertData[] values)
               {
                  for (int i = 0; i < values.Length; i++)
                  {
                     values[i] = new TED_FoliageData.TED_FoliageVertData();
                     values[i].mFoliageSetIndex = r.ReadInt32();
                     values[i].mFoliageSetBladeIndex = r.ReadInt32();
                  }
               }));


         }

         f.Close();

         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv23AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 23)
         {
            LoadTEDv22AndLater(f, versionNumber, ref dat, ref param);
            return;
         }



         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         //sim data
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);

         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightPassValues.SetEmptyValue(0);
         dat.mSimHeightPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         dat.mSimHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         dat.mTessellationOverrideValues.LoadByStripe(f,
            (JaggedContainer<byte>.LoadStripeDelegate)(delegate(BinaryReader r, byte[] values)
            {
               r.Read(values, 0, values.Length);
            }));


         int numXCameraPoints = f.ReadInt32();
         int numZCameraPoints = f.ReadInt32();
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues.SetEmptyValue(CameraHeightRep.cJaggedEmptyValue);
         dat.mCameraHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }


         //FOLIAGE
         int numFoliageSets = f.ReadInt32();
         if (numFoliageSets > 0)
         {
            dat.mFoliageData = new TED_FoliageData();
            dat.mFoliageData.mSetFileNames = new List<string>();
            for (int i = 0; i < numFoliageSets; i++)
            {
               dat.mFoliageData.mSetFileNames.Add(f.ReadString());
            }

            dat.mFoliageData.mFoliageVerts = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(numVerts);

            dat.mFoliageData.mFoliageVerts.LoadByStripe(f,
               (JaggedContainer<TED_FoliageData.TED_FoliageVertData>.LoadStripeDelegate)(delegate(BinaryReader r, TED_FoliageData.TED_FoliageVertData[] values)
               {
                  for (int i = 0; i < values.Length; i++)
                  {
                     values[i] = new TED_FoliageData.TED_FoliageVertData();
                     values[i].mFoliageSetIndex = r.ReadInt32();
                     values[i].mFoliageSetBladeIndex = r.ReadInt32();
                  }
               }));


         }

         f.Close();

         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv24AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (mShowAlerts)
            MessageBox.Show("WARNING! \n\n This map is older than version 25. It will need to be re-formated in order to be used.\n \n Please wait while this map loads and converts. \n Once this is complete, please save your file and check it back into perforce.");

         if (versionNumber < 24)
         {
            LoadTEDv23AndLater(f, versionNumber, ref dat, ref param);
            return;
         }



         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightPassValues.SetEmptyValue(0);
         dat.mSimHeightPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         dat.mSimHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         dat.mTessellationOverrideValues.LoadByStripe(f,
            (JaggedContainer<byte>.LoadStripeDelegate)(delegate(BinaryReader r, byte[] values)
            {
               r.Read(values, 0, values.Length);
            }));


         int numXCameraPoints = f.ReadInt32();
         int numZCameraPoints = f.ReadInt32();
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues.SetEmptyValue(CameraHeightRep.cJaggedEmptyValue);
         dat.mCameraHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);

         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues.SetEmptyValue(0);
         dat.mSimHeightBuildValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }


         //FOLIAGE
         int numFoliageSets = f.ReadInt32();
         if (numFoliageSets > 0)
         {
            dat.mFoliageData = new TED_FoliageData();
            dat.mFoliageData.mSetFileNames = new List<string>();
            for (int i = 0; i < numFoliageSets; i++)
            {
               dat.mFoliageData.mSetFileNames.Add(f.ReadString());
            }

            dat.mFoliageData.mFoliageVerts = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(numVerts);

            dat.mFoliageData.mFoliageVerts.LoadByStripe(f,
               (JaggedContainer<TED_FoliageData.TED_FoliageVertData>.LoadStripeDelegate)(delegate(BinaryReader r, TED_FoliageData.TED_FoliageVertData[] values)
               {
                  for (int i = 0; i < values.Length; i++)
                  {
                     values[i] = new TED_FoliageData.TED_FoliageVertData();
                     values[i].mFoliageSetIndex = r.ReadInt32();
                     values[i].mFoliageSetBladeIndex = r.ReadInt32();
                  }
               }));


         }

         f.Close();

         resamplePreV25Map(ref dat, ref param);
      }
      //----------------------------------------------
      static public void LoadTEDv25AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 25)
         {
            LoadTEDv24AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightPassValues.SetEmptyValue(0);
         dat.mSimHeightPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         dat.mSimHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         dat.mTessellationOverrideValues.LoadByStripe(f,
            (JaggedContainer<byte>.LoadStripeDelegate)(delegate(BinaryReader r, byte[] values)
            {
               r.Read(values, 0, values.Length);
            }));


         int numXCameraPoints = f.ReadInt32();
         int numZCameraPoints = f.ReadInt32();
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues.SetEmptyValue(CameraHeightRep.cJaggedEmptyValue);
         dat.mCameraHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);

         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues.SetEmptyValue(0);
         dat.mSimHeightBuildValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }


         //FOLIAGE
         int numFoliageSets = f.ReadInt32();
         if (numFoliageSets > 0)
         {
            dat.mFoliageData = new TED_FoliageData();
            dat.mFoliageData.mSetFileNames = new List<string>();
            for (int i = 0; i < numFoliageSets; i++)
            {
               dat.mFoliageData.mSetFileNames.Add(f.ReadString());
            }

            dat.mFoliageData.mFoliageVerts = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(numVerts);

            dat.mFoliageData.mFoliageVerts.LoadByStripe(f,
               (JaggedContainer<TED_FoliageData.TED_FoliageVertData>.LoadStripeDelegate)(delegate(BinaryReader r, TED_FoliageData.TED_FoliageVertData[] values)
               {
                  for (int i = 0; i < values.Length; i++)
                  {
                     values[i] = new TED_FoliageData.TED_FoliageVertData();
                     values[i].mFoliageSetIndex = r.ReadInt32();
                     values[i].mFoliageSetBladeIndex = r.ReadInt32();
                  }
               }));


         }

         f.Close();

         resamplePreV26Map(ref dat, ref param);
      }
      //----------------------------------------------
      //----------------------------------------------
      static public void LoadTEDv26AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 26)
         {
            LoadTEDv25AndLater(f, versionNumber, ref dat, ref param);
            return;
         }


         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightPassValues.SetEmptyValue(0);
         dat.mSimHeightPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         dat.mSimHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         dat.mTessellationOverrideValues.LoadByStripe(f,
            (JaggedContainer<byte>.LoadStripeDelegate)(delegate(BinaryReader r, byte[] values)
            {
               r.Read(values, 0, values.Length);
            }));


         int numXCameraPoints = f.ReadInt32();
         int numZCameraPoints = f.ReadInt32();
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues.SetEmptyValue(CameraHeightRep.cJaggedEmptyValue);
         dat.mCameraHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);

         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues.SetEmptyValue(0);
         dat.mSimHeightBuildValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);

         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }


         //FOLIAGE
         int numFoliageSets = f.ReadInt32();
         if (numFoliageSets > 0)
         {
            dat.mFoliageData = new TED_FoliageData();
            dat.mFoliageData.mSetFileNames = new List<string>();
            for (int i = 0; i < numFoliageSets; i++)
            {
               dat.mFoliageData.mSetFileNames.Add(f.ReadString());
            }

            dat.mFoliageData.mFoliageVerts = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(numVerts);

            dat.mFoliageData.mFoliageVerts.LoadByStripe(f,
               (JaggedContainer<TED_FoliageData.TED_FoliageVertData>.LoadStripeDelegate)(delegate(BinaryReader r, TED_FoliageData.TED_FoliageVertData[] values)
               {
                  for (int i = 0; i < values.Length; i++)
                  {
                     values[i] = new TED_FoliageData.TED_FoliageVertData();
                     values[i].mFoliageSetIndex = r.ReadInt32();
                     values[i].mFoliageSetBladeIndex = r.ReadInt32();
                  }
               }));


         }

         f.Close();

      }
       //----------------------------------------------
      static public void LoadTEDv27AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 27)
         {
            LoadTEDv26AndLater(f, versionNumber, ref dat, ref param);
            return;
         }


         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();


         dat.mAOValues = new float[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAOValues[i] = f.ReadSingle();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightPassValues.SetEmptyValue(0);
         dat.mSimHeightPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         dat.mSimHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         dat.mTessellationOverrideValues.LoadByStripe(f,
            (JaggedContainer<byte>.LoadStripeDelegate)(delegate(BinaryReader r, byte[] values)
            {
               r.Read(values, 0, values.Length);
            }));


         int numXCameraPoints = f.ReadInt32();
         int numZCameraPoints = f.ReadInt32();
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues.SetEmptyValue(CameraHeightRep.cJaggedEmptyValue);
         dat.mCameraHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);

         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues.SetEmptyValue(0);
         dat.mSimHeightBuildValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));
         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);


         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }


         //FOLIAGE
         int numFoliageSets = f.ReadInt32();
         if (numFoliageSets > 0)
         {
            dat.mFoliageData = new TED_FoliageData();
            dat.mFoliageData.mSetFileNames = new List<string>();
            for (int i = 0; i < numFoliageSets; i++)
            {
               dat.mFoliageData.mSetFileNames.Add(f.ReadString());
            }

            dat.mFoliageData.mFoliageVerts = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(numVerts);

            dat.mFoliageData.mFoliageVerts.LoadByStripe(f,
               (JaggedContainer<TED_FoliageData.TED_FoliageVertData>.LoadStripeDelegate)(delegate(BinaryReader r, TED_FoliageData.TED_FoliageVertData[] values)
               {
                  for (int i = 0; i < values.Length; i++)
                  {
                     values[i] = new TED_FoliageData.TED_FoliageVertData();
                     values[i].mFoliageSetIndex = r.ReadInt32();
                     values[i].mFoliageSetBladeIndex = r.ReadInt32();
                  }
               }));


         }

         f.Close();

      }
             //----------------------------------------------
      static public void LoadTEDv28AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 28)
         {
            LoadTEDv27AndLater(f, versionNumber, ref dat, ref param);
            return;
         }

         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();


         dat.mAOValues = new float[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAOValues[i] = f.ReadSingle();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightPassValues.SetEmptyValue(0);
         dat.mSimHeightPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         dat.mSimHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         dat.mTessellationOverrideValues.LoadByStripe(f,
            (JaggedContainer<byte>.LoadStripeDelegate)(delegate(BinaryReader r, byte[] values)
            {
               r.Read(values, 0, values.Length);
            }));


         int numXCameraPoints = f.ReadInt32();
         int numZCameraPoints = f.ReadInt32();
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues.SetEmptyValue(CameraHeightRep.cJaggedEmptyValue);
         dat.mCameraHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);

         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues.SetEmptyValue(0);
         dat.mSimHeightBuildValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues.SetEmptyValue(0);
         dat.mSimFloodPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues.SetEmptyValue(0);
         dat.mSimScarabPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));
         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);



         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }


         //FOLIAGE
         int numFoliageSets = f.ReadInt32();
         if (numFoliageSets > 0)
         {
            dat.mFoliageData = new TED_FoliageData();
            dat.mFoliageData.mSetFileNames = new List<string>();
            for (int i = 0; i < numFoliageSets; i++)
            {
               dat.mFoliageData.mSetFileNames.Add(f.ReadString());
            }

            dat.mFoliageData.mFoliageVerts = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(numVerts);

            dat.mFoliageData.mFoliageVerts.LoadByStripe(f,
               (JaggedContainer<TED_FoliageData.TED_FoliageVertData>.LoadStripeDelegate)(delegate(BinaryReader r, TED_FoliageData.TED_FoliageVertData[] values)
               {
                  for (int i = 0; i < values.Length; i++)
                  {
                     values[i] = new TED_FoliageData.TED_FoliageVertData();
                     values[i].mFoliageSetIndex = r.ReadInt32();
                     values[i].mFoliageSetBladeIndex = r.ReadInt32();
                  }
               }));


         }

         f.Close();

      }

               //----------------------------------------------
      static public void LoadTEDv29AndLater(BinaryReader f, int versionNumber, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         if (versionNumber < 29)
         {
            LoadTEDv28AndLater(f, versionNumber, ref dat, ref param);
            return;
         }
         // read header
         TDHeader head = new TDHeader();
         head.Read(f);

         param.initFromVisData(head.numVisXVerts, head.numVisZVerts, head.visTileSize, head.numVisVertsPerSimVert);

         int numXVerts = param.mNumVisXVerts;
         int numZVerts = param.mNumVisZVerts;
         int numVerts = numXVerts * numZVerts;


         //write our exportation params
         dat.mExportSettings = new Export360.ExportSettings();
         dat.mExportSettings.AmbientOcclusion = (f.ReadInt32() == 0 ? AmbientOcclusion.eAOQuality.cAO_Off : AmbientOcclusion.eAOQuality.cAO_Worst);
         dat.mExportSettings.RefineTerrain = (bool)(f.ReadInt32() == 0 ? false : true);
         dat.mExportSettings.RefineEpsilon = f.ReadSingle();
         dat.mExportSettings.RefineMinorityBias = f.ReadSingle();
         dat.mExportSettings.UniqueTexRes = f.ReadInt32();
         dat.mExportSettings.NavMeshQuantizationDist = f.ReadInt32();
         dat.mExportSettings.ObjectsInAO = (bool)(f.ReadInt32() == 0 ? false : true);

         // Read detail points
         dat.mPositions = new Vector3[numVerts];
         for (int i = 0; i < numVerts; i++)
         {
            dat.mPositions[i].X = f.ReadSingle();
            dat.mPositions[i].Y = f.ReadSingle();
            dat.mPositions[i].Z = f.ReadSingle();
         }

         // Fix edges
         sticthEdgesOfMap(ref dat.mPositions, numXVerts, numZVerts);

         dat.mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAlphaValues[i] = f.ReadByte();


         dat.mAOValues = new float[numVerts];
         for (int i = 0; i < numVerts; i++)
            dat.mAOValues[i] = f.ReadSingle();

         //sim data
         dat.mSimHeightPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightPassValues.SetEmptyValue(0);
         dat.mSimHeightPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         dat.mSimHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mTessellationOverrideValues = new JaggedContainer<byte>(numVerts);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         dat.mTessellationOverrideValues.LoadByStripe(f,
            (JaggedContainer<byte>.LoadStripeDelegate)(delegate(BinaryReader r, byte[] values)
            {
               r.Read(values, 0, values.Length);
            }));


         int numXCameraPoints = f.ReadInt32();
         int numZCameraPoints = f.ReadInt32();
         dat.mCameraHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mCameraHeightOverrideValues.SetEmptyValue(CameraHeightRep.cJaggedEmptyValue);
         dat.mCameraHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mFlightHeightGenAmt = f.ReadSingle();
         dat.mFlightHeightOverrideValues = new JaggedContainer<float>(numVerts);
         dat.mFlightHeightOverrideValues.SetEmptyValue(SimFlightRep.cJaggedEmptyValue);
         dat.mFlightHeightOverrideValues.LoadByStripe(f,
            (JaggedContainer<float>.LoadStripeDelegate)(delegate(BinaryReader r, float[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadSingle();
               }
            }));

         dat.mSimHeightBuildValues = new JaggedContainer<int>(numVerts);
         dat.mSimHeightBuildValues.SetEmptyValue(0);
         dat.mSimHeightBuildValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimFloodPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimFloodPassValues.SetEmptyValue(0);
         dat.mSimFloodPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimScarabPassValues = new JaggedContainer<int>(numVerts);
         dat.mSimScarabPassValues.SetEmptyValue(0);
         dat.mSimScarabPassValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));

         dat.mSimTileTypeValues = new JaggedContainer<int>(numVerts);



         int numXQuadNodes = (int)(param.mNumVisXVerts / BTerrainQuadNode.getMaxNodeWidth());
         dat.mTextureHolders = new TED_TextureDataQN[numXQuadNodes * numXQuadNodes];

         uint indexTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint alphaTexHeight = BTerrainTexturing.getAlphaTextureHeight();
         uint indexTexPitch = indexTexHeight * 2;
         uint alphaTexPitch = alphaTexHeight * 2; ;

         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            dat.mTextureHolders[i] = new TED_TextureDataQN();

            dat.mTextureHolders[i].mXIndex = f.ReadInt32();
            dat.mTextureHolders[i].mZIndex = f.ReadInt32();
            dat.mTextureHolders[i].mLayers = new List<BTerrainTexturingLayer>();
            int numLayers = f.ReadInt32();
            for (int k = 0; k < numLayers; k++)
            {
               int ck = dat.mTextureHolders[i].mLayers.Count;
               dat.mTextureHolders[i].mLayers.Add(new BTerrainTexturingLayer());

               dat.mTextureHolders[i].mLayers[ck].mLayerType = (BTerrainTexturingLayer.eLayerType)f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mActiveTextureIndex = f.ReadInt32();
               dat.mTextureHolders[i].mLayers[ck].mAlphaLayer = f.ReadBytes((int)(BTerrainQuadNode.cMaxWidth * BTerrainQuadNode.cMaxHeight));
            }
         }

         // Read texture names
         SimTerrainType.mActiveWorkingSet.Clear();

         try
         {
            int numActiveSet = f.ReadInt32();
            dat.mActiveTextures = new TED_ActiveTextureData[numActiveSet];
            {
               for (int i = 0; i < numActiveSet; i++)
               {
                  dat.mActiveTextures[i] = new TED_ActiveTextureData();
                  dat.mActiveTextures[i].mFilename = f.ReadString();
                  dat.mActiveTextures[i].mUScale = f.ReadInt32();
                  dat.mActiveTextures[i].mVScale = f.ReadInt32();
                  dat.mActiveTextures[i].mBlendOp = f.ReadInt32();
                  dat.mActiveTextures[i].mSpecPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelBias = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelPower = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelScale = f.ReadSingle();
                  dat.mActiveTextures[i].mFresnelRefractPerc = f.ReadSingle();
               }
            }

            // Read Decal Names
            TerrainGlobals.getTexturing().destroyActiveDecals();
            int numActiveDecals = f.ReadInt32();
            if (numActiveDecals != 0)
            {
               dat.mActiveDecals = new TED_ActiveDecalData[numActiveDecals];
               for (int i = 0; i < numActiveDecals; i++)
               {
                  dat.mActiveDecals[i] = new TED_ActiveDecalData();
                  dat.mActiveDecals[i].mFilename = f.ReadString();
               }
            }



            // Read Decal Instances
            int numActiveDecalInstances = f.ReadInt32();
            dat.mActiveDecalInstances = new TED_ActiveDecalInstanceData[numActiveDecalInstances];
            if (numActiveDecalInstances != 0)
            {
               for (int i = 0; i < numActiveDecalInstances; i++)
               {
                  dat.mActiveDecalInstances[i] = new TED_ActiveDecalInstanceData();
                  dat.mActiveDecalInstances[i].mActiveDecalIndex = f.ReadInt32();
                  dat.mActiveDecalInstances[i].mRotation = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMax.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.X = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileBoundsMin.Y = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterX = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mTileCenterY = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mUScale = f.ReadSingle();
                  dat.mActiveDecalInstances[i].mVScale = f.ReadSingle();
               }
            }
         }
         catch
         {
            dat.mActiveTextures = null;
            dat.mActiveDecalInstances = null;
            dat.mActiveDecals = null;
            MessageBox.Show("There was a problems reading the textures from your file. \n This is usually a result of a bad save.");
         }


         //ROADS
         int numRoads = f.ReadInt32();
         dat.mRoads = new TED_RoadData[numRoads];
         for (int i = 0; i < numRoads; i++)
         {
            dat.mRoads[i] = new TED_RoadData();
            dat.mRoads[i].mTextureName = f.ReadString();
            dat.mRoads[i].mWidth = f.ReadSingle();
            dat.mRoads[i].mTessSize = f.ReadSingle();

            int numControlPts = f.ReadInt32();
            dat.mRoads[i].mControlPoints = new roadControlPoint[numControlPts];
            for (int k = 0; k < numControlPts; k++)
            {
               dat.mRoads[i].mControlPoints[k] = new roadControlPoint();
               dat.mRoads[i].mControlPoints[k].mControlPointType = (roadControlPoint.eControlPointType)f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridX = f.ReadInt32();
               dat.mRoads[i].mControlPoints[k].mGridY = f.ReadInt32();
            }

            int numSegments = f.ReadInt32();
            dat.mRoads[i].mSegments = new TED_RoadData.TED_RoadData_Segment[numSegments];
            for (int k = 0; k < numSegments; k++)
            {
               dat.mRoads[i].mSegments[k] = new TED_RoadData.TED_RoadData_Segment();
               dat.mRoads[i].mSegments[k].mA.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mA.Y = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.X = f.ReadInt32();
               dat.mRoads[i].mSegments[k].mB.Y = f.ReadInt32();
            }
         }


         //FOLIAGE
         int numFoliageSets = f.ReadInt32();
         if (numFoliageSets > 0)
         {
            dat.mFoliageData = new TED_FoliageData();
            dat.mFoliageData.mSetFileNames = new List<string>();
            for (int i = 0; i < numFoliageSets; i++)
            {
               dat.mFoliageData.mSetFileNames.Add(f.ReadString());
            }

            dat.mFoliageData.mFoliageVerts = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(numVerts);

            dat.mFoliageData.mFoliageVerts.LoadByStripe(f,
               (JaggedContainer<TED_FoliageData.TED_FoliageVertData>.LoadStripeDelegate)(delegate(BinaryReader r, TED_FoliageData.TED_FoliageVertData[] values)
               {
                  for (int i = 0; i < values.Length; i++)
                  {
                     values[i] = new TED_FoliageData.TED_FoliageVertData();
                     values[i].mFoliageSetIndex = r.ReadInt32();
                     values[i].mFoliageSetBladeIndex = r.ReadInt32();
                  }
               }));


         }

         f.Close();

      }
   }
}