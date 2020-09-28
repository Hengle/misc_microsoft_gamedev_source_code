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
   #region TED Loading Saving
   public partial class TEDIO
   {
      static public int cTEDVersion =30;
      static public bool mShowAlerts = true;

      public struct TDHeader
      {
         public int numVisXVerts;
         public int numVisZVerts;
         public float visTileSize;
         public float numVisVertsPerSimVert;


         public void Write(BinaryWriter w)
         {
            w.Write(numVisXVerts);
            w.Write(numVisZVerts);
            w.Write(visTileSize);
            w.Write(numVisVertsPerSimVert);
         }
         public void Read(BinaryReader r)
         {
            numVisXVerts = r.ReadInt32();
            numVisZVerts = r.ReadInt32();
            visTileSize = r.ReadSingle();
            numVisVertsPerSimVert = r.ReadSingle();
         }
      }

      public class TED_TextureDataQN : BTerrainLayerContainer
      {
         ~TED_TextureDataQN()
         {
            base.destroy();
         }
         public long mXIndex=0;
         public long mZIndex = 0;
      }
      public class TED_ActiveTextureData
      {
         public string mFilename;
         public int mUScale=2;
         public int mVScale=2;
         public int mBlendOp=0;
         public float mSpecPower = 25;
         public float mFresnelBias = 0;
         public float mFresnelPower = 3.5f;
         public float mFresnelScale = 20;
         public float mFresnelRefractPerc = 0.75f;
      }
      public class TED_ActiveDecalData
      {
         public string mFilename;
      }
      public class TED_ActiveDecalInstanceData
      {
         public int mActiveDecalIndex;
         public float mRotation;
         public float mUScale;
         public float mVScale;
         public float mTileCenterX;
         public float mTileCenterY;
         public Vector2 mTileBoundsMax;
         public Vector2 mTileBoundsMin;
      }
      public class TED_RoadData
      {
         public string mTextureName;
         public float mWidth;
         public float mTessSize;

         public roadControlPoint[] mControlPoints;
         public class TED_RoadData_Segment
         {
            public Point mA = new Point();
            public Point mB = new Point();
         }
         public TED_RoadData_Segment[] mSegments;
      }
      public class TED_FoliageData
      {
         public class TED_FoliageVertData
         {
            public int mFoliageSetIndex = -1;
            public int mFoliageSetBladeIndex = -1;
            public bool compare(TED_FoliageVertData a)
            {
               return a.mFoliageSetIndex == mFoliageSetIndex && a.mFoliageSetBladeIndex == mFoliageSetBladeIndex;
            }
         };
         public void destroy()
         {
            mFoliageVerts.Clear();
            mFoliageVerts = null;
         }
         public List<string> mSetFileNames= null;
         public JaggedContainer<TED_FoliageVertData> mFoliageVerts;
      }
      public struct TEDLoadedData
      {
         public void init()
         {
            mFlightHeightGenAmt = 16.0f;
         }

         public void destroy()
         {
            mPositions = null;
            mTextureHolders = null;
            mActiveTextures = null;
            mActiveDecals = null;
            mActiveDecalInstances = null;
            mAlphaValues = null;
            mSimHeightPassValues = null;
            mSimHeightBuildValues = null;
            mSimHeightOverrideValues = null;
            mTessellationOverrideValues = null;
            mCameraHeightOverrideValues = null;
            mFlightHeightOverrideValues = null;
            mRoads = null;
            mFoliageData = null;
            mAOValues = null;
         }
         public Vector3 []mPositions;
         public TED_TextureDataQN[] mTextureHolders;
         public Export360.ExportSettings mExportSettings;
         public TED_ActiveTextureData[] mActiveTextures;
         public TED_ActiveDecalData[] mActiveDecals;
         public TED_ActiveDecalInstanceData[] mActiveDecalInstances;
         public byte[] mAlphaValues;
         public float[] mAOValues;
         
         public JaggedContainer<int> mSimHeightPassValues;
         public JaggedContainer<int> mSimHeightBuildValues;
         public JaggedContainer<int> mSimFloodPassValues;
         public JaggedContainer<int> mSimScarabPassValues;
         public JaggedContainer<int> mSimTileTypeValues;
         public JaggedContainer<float> mSimHeightOverrideValues;
         public JaggedContainer<byte> mTessellationOverrideValues;
         public JaggedContainer<float> mCameraHeightOverrideValues;
         public JaggedContainer<float> mFlightHeightOverrideValues;
         public float mFlightHeightGenAmt;

         public TED_RoadData[] mRoads;

         public TED_FoliageData mFoliageData;

      }

      static public void createTexturingFromTED(TEDLoadedData dat, int xVertOffset, int zVertOffset)
      {
         TerrainGlobals.getTexturing().destroyActiveDecals();

         //active textures
         if (dat.mActiveTextures != null)
         {
            //first, set our sim properties
            for (int i = 0; i < dat.mActiveTextures.Length; i++)
            {
               TerrainTextureDef ttd = SimTerrainType.getFromTypeName(dat.mActiveTextures[i].mFilename);
               if (ttd == null)
               {
                  if (mShowAlerts)
                     MessageBox.Show("WARNING: Terrain type " + dat.mActiveTextures[i].mFilename + " not found! Replacing with a default type");
                  ttd = SimTerrainType.getFromTypeName(SimTerrainType.mBlankTex);
                  if (ttd == null)
                  {
                     SimTerrainType.addDefaultBlankType();
                     ttd = SimTerrainType.getFromTypeName(SimTerrainType.mBlankTex);
                  }
               }
               int ind = SimTerrainType.getIndexFromDef(ttd);
               SimTerrainType.mActiveWorkingSet.Add(ind);
            }
            //now set our visual properties
            TerrainGlobals.getTexturing().createTexturesFromSimDefs();
            for (int i = 0; i < dat.mActiveTextures.Length; i++)
            {
               BTerrainActiveTextureContainer act = TerrainGlobals.getTexturing().getActiveTexture(i);
               act.mUScale = dat.mActiveTextures[i].mUScale;
               act.mVScale = dat.mActiveTextures[i].mVScale;
               act.mBlendOp = dat.mActiveTextures[i].mBlendOp;

               act = TerrainGlobals.getTexturing().getActiveTexture(i);
               act.mBlendOp = dat.mActiveTextures[i].mBlendOp;

            }
         }
         else
         {
            TerrainTextureDef ttd = SimTerrainType.getFromTypeName(SimTerrainType.mBlankTex);
            if (ttd == null)
            {
               SimTerrainType.addDefaultBlankType();
               ttd = SimTerrainType.getFromTypeName(SimTerrainType.mBlankTex);
            }
            int ind = SimTerrainType.getIndexFromDef(ttd);
            SimTerrainType.mActiveWorkingSet.Add(ind);

            TerrainGlobals.getTexturing().createTexturesFromSimDefs();
         }


         //decals 
         if (dat.mActiveDecals != null)
         {
            for (int i = 0; i < dat.mActiveDecals.Length; i++)
            {
               //strip out any crazy extra pathing data
               String fname = dat.mActiveDecals[i].mFilename;
               int k = fname.LastIndexOf("terrain\\") + 7;
               fname = fname.Substring(k, fname.Length - k);
               fname = CoreGlobals.getWorkPaths().mTerrainTexturesPath + fname;

               TerrainGlobals.getTexturing().addActiveDecal(fname);
            }
         }

         //decal instances
         if (dat.mActiveDecalInstances != null)
         {
            for (int i = 0; i < dat.mActiveDecalInstances.Length; i++)
            {
               TerrainGlobals.getTexturing().addActiveDecalInstance(dat.mActiveDecalInstances[i].mActiveDecalIndex,
                                                                     (int)dat.mActiveDecalInstances[i].mTileCenterX + xVertOffset,
                                                                     (int)dat.mActiveDecalInstances[i].mTileCenterY + zVertOffset,
                                                                     dat.mActiveDecalInstances[i].mUScale,
                                                                     dat.mActiveDecalInstances[i].mVScale,
                                                                     dat.mActiveDecalInstances[i].mRotation,
                                                                     (int)dat.mActiveDecalInstances[i].mTileBoundsMin.X + xVertOffset,
                                                                     (int)dat.mActiveDecalInstances[i].mTileBoundsMax.X + xVertOffset,
                                                                     (int)dat.mActiveDecalInstances[i].mTileBoundsMin.Y + zVertOffset,
                                                                     (int)dat.mActiveDecalInstances[i].mTileBoundsMax.Y + zVertOffset);
            }
         }
      }

      static void createQNContainerFromTed(TED_TextureDataQN dat,BTerrainQuadNode node)
      {
         bool[] usedTexSlots = new bool[TerrainGlobals.getTexturing().getActiveTextureCount()];

         if (dat.mLayers == null)
         {
            node.mLayerContainer.makeBaseLayer(0);
            usedTexSlots[0] = true;
         }
         else
         {
            for (int k = 0; k < dat.mLayers.Count; k++)
            {
               if (dat.mLayers[k].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
               {
                  if (dat.mLayers[k].mActiveTextureIndex < 0 ||
                     dat.mLayers[k].mActiveTextureIndex >= TerrainGlobals.getTexturing().getActiveTextureCount())
                     continue;

                  if(usedTexSlots[dat.mLayers[k].mActiveTextureIndex])
                     continue;
                  usedTexSlots[dat.mLayers[k].mActiveTextureIndex] = true;


                  node.mLayerContainer.newSplatLayer(dat.mLayers[k].mActiveTextureIndex,false);
               }
               else if (dat.mLayers[k].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
               {
                  if (dat.mLayers[k].mActiveTextureIndex < 0 ||
                     dat.mLayers[k].mActiveTextureIndex >= TerrainGlobals.getTexturing().getActiveDecalInstancesCount())
                     continue;


                  node.mLayerContainer.newDecalLayer(dat.mLayers[k].mActiveTextureIndex);
               }
               BTerrainTexturingLayer tLayer = node.mLayerContainer.giveTopmostLayer();
               tLayer.mAlphaLayer = new byte[BTerrainQuadNode.cMaxHeight * BTerrainQuadNode.cMaxWidth];
               dat.mLayers[k].mAlphaLayer.CopyTo(tLayer.mAlphaLayer, 0);

            }
         }

         BTerrainQuadNodeDesc desc = node.getDesc();
         if(desc.mMinXVert == 8 * BTerrainQuadNode.cMaxWidth &&
            desc.mMinZVert == 8 * BTerrainQuadNode.cMaxWidth)
            usedTexSlots[0] = true;

         //we're now gaurenteed that each qn will have the same number of layers
         //so, let's do the sorting dance..
         node.mLayerContainer.ensureProperLayerOrdering();


      }
      static public void createQNTexuringFromTED(TEDLoadedData dat, int xVertOffset, int zVertOffset)
      {
         //[02.06.07] CLM - this is slower, but a million times more accurate.

         
         for (int i = 0; i < dat.mTextureHolders.Length; i++)
         {
            

            int xPt = (int)(dat.mTextureHolders[i].mXIndex * BTerrainQuadNode.cMaxWidth + xVertOffset + 2);
            int yPt = (int)(dat.mTextureHolders[i].mZIndex * BTerrainQuadNode.cMaxHeight + zVertOffset + 2);
            BTerrainQuadNode node = TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafNodeContainingPoint(xPt, yPt);
            if (node == null)
               continue;
            if (node.mLayerContainer == null)
               node.mLayerContainer = TerrainGlobals.getTexturing().getNewContainer();
            TED_TextureDataQN tDat = dat.mTextureHolders[i];
            createQNContainerFromTed(tDat, node);
         }

         BTerrainQuadNode[] nodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
         for (int i = 0; i < nodes.Length; i++)
         {
            if (nodes[i].mLayerContainer == null)
               nodes[i].mLayerContainer = TerrainGlobals.getTexturing().getNewContainer();
         }
      }
      static public void createRoadsFromTED(TEDLoadedData dat)
      {
         if (dat.mRoads == null)
            return;

         for (int i = 0; i < dat.mRoads.Length; i++)
         {
            RoadManager.addNewRoad(dat.mRoads[i].mTextureName, dat.mRoads[i].mWidth);
            Road rd = RoadManager.giveRoad(i);
            rd.setWorldTesselationSize(dat.mRoads[i].mTessSize);

            for (int k = 0; k < dat.mRoads[i].mControlPoints.Length; k++)
            {
               rd.addPoint(dat.mRoads[i].mControlPoints[k].mGridX, dat.mRoads[i].mControlPoints[k].mGridY, dat.mRoads[i].mControlPoints[k].mControlPointType);
            }

            for (int k = 0; k < dat.mRoads[i].mSegments.Length; k++)
            {
               rd.addSegment(dat.mRoads[i].mSegments[k].mA, dat.mRoads[i].mSegments[k].mB, roadControlPoint.eControlPointType.cAngled);
            }
            rd.rebuildVisuals();
         }
      }
      static public void createFoliageFromTed(TEDLoadedData dat)
      {
         if (dat.mFoliageData == null)
            return;

         for (int i = 0; i < dat.mFoliageData.mSetFileNames.Count; i++)
         {
            string fullName = CoreGlobals.getWorkPaths().mFoliagePath + "\\" + dat.mFoliageData.mSetFileNames[i];
            FoliageManager.newSet(fullName);
         }


         long id;
         TED_FoliageData.TED_FoliageVertData maskValue;
         dat.mFoliageData.mFoliageVerts.ResetIterator();
         while (dat.mFoliageData.mFoliageVerts.MoveNext(out id, out maskValue))
         {
            if (maskValue.mFoliageSetIndex==-1)
               continue;

            FoliageSet fs = FoliageManager.giveSet(maskValue.mFoliageSetIndex);
            if (fs != null)
            {
               int x = (int)id / TerrainGlobals.getTerrain().getNumXVerts();
               int z = (int)id - x * TerrainGlobals.getTerrain().getNumXVerts();
               id = x + TerrainGlobals.getTerrain().getNumXVerts() * z;
               FoliageManager.setBladeToGrid((int)id, fs.mFullFileName, maskValue.mFoliageSetBladeIndex, false, false);
            }
         }

         FoliageManager.removeUnusedSets();

      }
      static public void createAllFromTed(TEDLoadedData dat, TerrainCreationParams param)
      {
         createTexturingFromTED(dat, 0, 0);

         TerrainGlobals.getTerrain().createFromTED(param, dat.mPositions, dat.mAlphaValues,dat.mAOValues,
            dat.mSimHeightBuildValues, 
            dat.mSimHeightPassValues, 
            dat.mSimHeightOverrideValues, 
            dat.mTessellationOverrideValues, 
            dat.mCameraHeightOverrideValues,
            dat.mFlightHeightOverrideValues,
            dat.mSimFloodPassValues,
            dat.mSimScarabPassValues,
            dat.mSimTileTypeValues);

         TerrainGlobals.getEditor().getSimRep().getFlightRep().setHeightGenOffset(dat.mFlightHeightGenAmt);

         createQNTexuringFromTED(dat, 0, 0);

         TerrainGlobals.getTerrain().setExportSettings(dat.mExportSettings);

         createRoadsFromTED(dat);
         createFoliageFromTed(dat);

         dat.destroy();
      }

      #region RESIZE TED DATA GENERIC
      static public void resampleMap(ref TEDLoadedData dat, ref TerrainCreationParams param, ref TerrainCreationParams newParam)
      {
         int oldSizeX = param.mNumVisXVerts;
         int oldSizeZ = param.mNumVisZVerts;

         //CLM what about non POW2 maps?!?!
         int newSizeX = newParam.mNumVisXVerts;
         int newSizeZ = newParam.mNumVisZVerts;



         resampleVertexData(ref dat, ref param, newSizeX, newSizeZ);
         GC.WaitForPendingFinalizers();
         GC.Collect();

         resampleTextureData(ref dat, ref param, newSizeX, newSizeZ);
         GC.WaitForPendingFinalizers();
         GC.Collect();

         param.initFromVisData(newSizeX, newSizeZ, TerrainCreationParams.cTileSpacingOne, 8);//(float)eSimQuality.cQuality_Min);
      }

      static void resampleVertexData(ref TEDLoadedData dat, ref TerrainCreationParams param, int newSizeX, int newSizeZ)
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

         //AO VALUES
         dat.mAOValues = ImageManipulation.resizeF32Img(dat.mAOValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, FilterType);

         dat.mSimHeightPassValues.SetEmptyValue(0);
         dat.mSimHeightOverrideValues.SetEmptyValue(SimHeightRep.cJaggedEmptyValue);
         dat.mTessellationOverrideValues.SetEmptyValue(BTerrainEditor.cTesselationEmptyVal);
         dat.mSimHeightBuildValues.SetEmptyValue(0);
         dat.mCameraHeightOverrideValues.SetEmptyValue(CameraHeightRep.cJaggedEmptyValue);
         dat.mFlightHeightOverrideValues.SetEmptyValue(SimFlightRep.cJaggedEmptyValue);

         resampleJaggedArrayInt(ref dat.mSimHeightPassValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, 0, false);
         resampleJaggedArrayFloat(ref dat.mSimHeightOverrideValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, SimHeightRep.cJaggedEmptyValue, false);
         resampleJaggedArrayByte(ref dat.mTessellationOverrideValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, BTerrainEditor.cTesselationEmptyVal, false);
         resampleJaggedArrayInt(ref dat.mSimHeightBuildValues, param.mNumVisXVerts, param.mNumVisZVerts, newSizeX, newSizeZ, 0, false);

         //Camera/flight is a seperate sparce data set
         int oldCameraX = param.mNumVisXVerts >> 6;
         int oldCameraZ = param.mNumVisZVerts >> 6;
         int newCameraX = newSizeX >> 6;
         int newCameraZ = newSizeZ >> 6;
         resampleJaggedArrayFloat(ref dat.mCameraHeightOverrideValues, oldCameraX, oldCameraZ, newCameraX, newCameraZ, CameraHeightRep.cJaggedEmptyValue, false);
         resampleJaggedArrayFloat(ref dat.mFlightHeightOverrideValues, oldCameraX, oldCameraZ, newCameraX, newCameraZ, SimFlightRep.cJaggedEmptyValue, false);

         resampleFoliage(ref dat, ref param, newSizeX, newSizeZ);
         resampleRoads(ref dat, ref param, newSizeX, newSizeZ);
      }
      static void resampleJaggedArrayFloat(ref JaggedContainer<float> dat, int origX, int origY, int newX, int newY, float emptyVal, bool transpose)
      {
         float[] oldArry = new float[origX * origY];

         if (transpose)
         {
            int c = 0;
            for (int i = 0; i < origX; i++)
            {
               for (int j = 0; j < origY; j++)
               {
                  int idx = i + origX * j;
                  oldArry[c++] = dat.GetValue(idx);
               }
            }
         }
         else
         {
            for (int i = 0; i < origX * origY; i++)
               oldArry[i] = dat.GetValue(i);
         }



         float[] imgScaledX = new float[newX * newY];
         ImageManipulation.resizeF32Img(oldArry, imgScaledX, origX, origY, newX, newY, ImageManipulation.eFilterType.cFilter_Nearest);


         dat = null;
         dat = new JaggedContainer<float>(newX * newY);
         dat.SetEmptyValue(emptyVal);

         for (int i = 0; i < newX * newY; i++)
         {
            if (imgScaledX[i] != (float)emptyVal)
               dat.SetValue(i, (float)imgScaledX[i]);
         }

         imgScaledX = null;
         oldArry = null;
      }
      static void resampleJaggedArrayByte(ref JaggedContainer<byte> dat, int origX, int origY, int newX, int newY, byte emptyVal, bool transpose)
      {
         byte[] oldArry = new byte[origX * origY];
         if (transpose)
         {
            int c = 0;
            for (int i = 0; i < origX; i++)
            {
               for (int j = 0; j < origY; j++)
               {
                  int idx = i + origX * j;
                  oldArry[c++] = dat.GetValue(idx);
               }
            }
         }
         else
         {
            for (int i = 0; i < origX * origY; i++)
               oldArry[i] = dat.GetValue(i);
         }


         byte[] imgScaledX = new byte[newX * newY];
         ImageManipulation.resizeGreyScaleImg(oldArry, imgScaledX, origX, origY, newX, newY, ImageManipulation.eFilterType.cFilter_Nearest);


         dat = null;
         dat = new JaggedContainer<byte>(newX * newY);
         dat.SetEmptyValue(emptyVal);

         for (int i = 0; i < newX * newY; i++)
         {
            if (imgScaledX[i] != (byte)emptyVal)
               dat.SetValue(i, (byte)imgScaledX[i]);
         }

         imgScaledX = null;
         oldArry = null;
      }
      static void resampleJaggedArrayInt(ref JaggedContainer<int> dat, int origX, int origY, int newX, int newY, byte emptyVal, bool transpose)
      {
         float[] oldArry = new float[origX * origY];
         if (transpose)
         {
            int c = 0;
            for (int i = 0; i < origX; i++)
            {
               for (int j = 0; j < origY; j++)
               {
                  int idx = i + origX * j;
                  oldArry[c++] = dat.GetValue(idx);
               }
            }
         }
         else
         {
            for (int i = 0; i < origX * origY; i++)
               oldArry[i] = dat.GetValue(i);
         }



         float[] imgScaledX = new float[newX * newY];
         ImageManipulation.resizeF32Img(oldArry, imgScaledX, origX, origY, newX, newY, ImageManipulation.eFilterType.cFilter_Nearest);


         dat = null;
         dat = new JaggedContainer<int>(newX * newY);
         dat.SetEmptyValue(emptyVal);

         for (int i = 0; i < newX * newY; i++)
         {
            if (imgScaledX[i] != (byte)emptyVal)
               dat.SetValue(i, (int)imgScaledX[i]);
         }


         imgScaledX = null;
         oldArry = null;
      }
      static void resampleFoliage(ref TEDLoadedData dat, ref TerrainCreationParams param, int newX, int newY)
      {
         if (dat.mFoliageData == null || dat.mFoliageData.mFoliageVerts == null)
            return;

         JaggedContainer<TED_FoliageData.TED_FoliageVertData> newFoliage = new JaggedContainer<TED_FoliageData.TED_FoliageVertData>(newX * newY);

         long id;
         TED_FoliageData.TED_FoliageVertData maskValue;
         dat.mFoliageData.mFoliageVerts.ResetIterator();
         while (dat.mFoliageData.mFoliageVerts.MoveNext(out id, out maskValue))
         {
            int x = (int)id / param.mNumVisXVerts;
            int z = (int)id - x * param.mNumVisZVerts;

            //ignore every other grassblade (this should be sufficient..)
            if (x % 2 == 1 || z % 2 == 1)
               continue;

            long newIndex = ((x >> 1) * newX) + (z >> 1);
            newFoliage.SetValue(newIndex, maskValue);
         }

         dat.mFoliageData.mFoliageVerts.Clear();
         dat.mFoliageData.mFoliageVerts = null;
         dat.mFoliageData.mFoliageVerts = newFoliage; //clm will this pointer persist??


      }
      static void resampleRoads(ref TEDLoadedData dat, ref TerrainCreationParams param, int newX, int newY)
      {
         //Hu?

      }
      static void resampleTextureData(ref TEDLoadedData dat, ref TerrainCreationParams param, int newSizeX, int newSizeZ)
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

      //----------------------------------------------
      static public void LoadTEDvLatest(string filename, ref TEDLoadedData dat, ref TerrainCreationParams param)
      {
         Stream stream = null;
         bool isZip = false;// true;   //AFO no longer trying to load zip files.  they are not used.

         DiskFile zipFile = new DiskFile(filename);
         if (!zipFile.Exists)
         {
            return;
         }
         if (isZip == true)
         {
            try
            {
               ZipArchive zip = new ZipArchive(zipFile);
               //AbstractFile af = zip.GetFile(Path.GetFileName(filename));
               //stream = af.OpenRead();
               AbstractFile[] files = zip.GetFiles(true, null);
               if (files.Length > 0)
               {
                  AbstractFile af = files[0];
                  stream = af.OpenRead();
               }
            }
            catch (Xceed.Zip.InvalidZipStructureException ex) //Not a zip file?
            {
               ex.ToString();
               isZip = false;
            }
         }
         if (isZip == false)
         {
            stream = File.Open(filename, FileMode.Open, FileAccess.Read);
         }
         //BufferedStream bs = new BufferedStream(stream, 1024);  //~20mb buffer

         BinaryReader f = new BinaryReader(stream);

         int versionNumber;

         // read version number
         versionNumber = f.ReadInt32();
         if (versionNumber < cTEDVersion)
         {
            if (mShowAlerts)
               MessageBox.Show("This TED file is out of date.Please re-save these files, and check back into perforce.");
            LoadTEDv29AndLater(f, versionNumber, ref dat, ref param);
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

            if (float.IsNaN(dat.mPositions[i].X)) dat.mPositions[i].X = 0;
            if (float.IsNaN(dat.mPositions[i].Y)) dat.mPositions[i].Y = 0;
            if (float.IsNaN(dat.mPositions[i].Z)) dat.mPositions[i].Z = 0;
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
         dat.mSimTileTypeValues.SetEmptyValue(0);
         dat.mSimTileTypeValues.LoadByStripe(f,
            (JaggedContainer<int>.LoadStripeDelegate)(delegate(BinaryReader r, int[] values)
            {
               for (int i = 0; i < values.Length; i++)
               {
                  values[i] = r.ReadInt32();
               }
            }));


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
      static public void LoadTEDFile(string filename)
      {
         using (PerfSection p = new PerfSection("LoadTEDFile"))
         {
            TerrainCreationParams param = new TerrainCreationParams();
            TEDLoadedData dat = new TEDLoadedData();
            dat.init();

            mShowAlerts = !CoreGlobals.IsBatchExport;
            using (PerfSection p2 = new PerfSection("LoadTEDvLatest"))
            {
               LoadTEDvLatest(filename, ref dat, ref param);
            }
            using (PerfSection p3 = new PerfSection("createAllFromTed"))
            {
               createAllFromTed(dat, param);
            }
         }
      }
      static bool sSaveAsZip = false;
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      static public void SaveTEDFile(string filename)
      {
         using (PerfSection p = new PerfSection("SaveTEDFile"))
         {
            int versionNumber = cTEDVersion;

            TDHeader head;
            head.numVisXVerts = TerrainGlobals.getTerrain().getNumXVerts();
            head.numVisZVerts = TerrainGlobals.getTerrain().getNumZVerts();
            head.visTileSize = TerrainGlobals.getTerrain().getTileScale();
            head.numVisVertsPerSimVert = 1.0f / (float)TerrainGlobals.getEditor().getSimRep().getVisToSimScale();
            //head.numSimXTiles = TerrainGlobals.getEditor().getSimRep().getNumXTiles();
            //head.numSimZTiles = TerrainGlobals.getEditor().getSimRep().getNumXTiles();
            //head.simTileSize = TerrainGlobals.getEditor().getSimRep().getTileScale();
            //head.simToVisMultiplier = (int)(1.0f/TerrainGlobals.getEditor().getSimRep().getVisToSimScale());
            try
            {

               if (File.Exists(filename))
                  File.Delete(filename);

            }
            catch (System.UnauthorizedAccessException ex)
            {
               MessageBox.Show("Error saving " + filename + " make sure this file is checked out from perforce.");
               return;
            }
            //filename = Path.ChangeExtension(filename, ".zed");
            Stream s;
            //if (sSaveAsZip)
            //{
            //   DiskFile zipFile = new DiskFile(filename);
            //   zipFile.Create();
            //   ZipArchive zip = new ZipArchive(zipFile);
            //   AbstractFile md = zip.CreateFile(Path.GetFileName(filename), true);
            //   s = md.OpenWrite(true);
            //}
            //else
            {
               s = File.Open(filename, FileMode.OpenOrCreate, FileAccess.Write);
            }
            //MemoryStream ms = new MemoryStream();
            BinaryWriter f = null;
            BufferedStream bs = null;
            if (CoreGlobals.OutOfMemory == false)
            {
               bs = new BufferedStream(s, 20000000);  //~20mb buffer
               f = new BinaryWriter(bs);
            }
            else
            {
               f = new BinaryWriter(s);
               CoreGlobals.ShowMessage("Not enough memory for fast save.  Please let save finish then close the app");

            }
            try
            {
               // write version number first
               f.Write(versionNumber);
            }
            catch (System.OutOfMemoryException ex)
            {
               CoreGlobals.OutOfMemory = true;
               CoreGlobals.getErrorManager().LogErrorToNetwork(ex.ToString());
               bs = null;
               try
               {
                  s.Close();
               }
               catch (System.Exception ex2)
               {

               }

               s = File.Open(filename, FileMode.OpenOrCreate, FileAccess.Write);
               //out of memory try again.
               f = new BinaryWriter(s);
               // write version number first
               f.Write(versionNumber);
               CoreGlobals.ShowMessage("Not enough memory for fast save.  Please let save finish then close the app");


            }

            try
            {

               // write header
               head.Write(f);

               //write our exportation params
               Export360.ExportSettings es = TerrainGlobals.getTerrain().getExportSettings();
               f.Write((int)es.AmbientOcclusion);
               f.Write((int)(es.RefineTerrain ? 1 : 0));
               f.Write(es.RefineEpsilon);
               f.Write(es.RefineMinorityBias);
               f.Write(es.UniqueTexRes);
               f.Write(es.NavMeshQuantizationDist);
               f.Write((int)(es.ObjectsInAO ? 1 : 0));

               // write out detail points
               int numVerts = TerrainGlobals.getTerrain().getNumXVerts() * TerrainGlobals.getTerrain().getNumZVerts();

               byte[] alphaVals = TerrainGlobals.getEditor().getAlphaValues();
               float[] aoVals = TerrainGlobals.getEditor().getAmbientOcclusionValues();
               Vector3[] detailPoints = TerrainGlobals.getEditor().getDetailPoints();

               using (PerfSection p1 = new PerfSection("sticthEdgesOfMap"))
               {
                  // Fix edges
                  sticthEdgesOfMap(ref detailPoints, head.numVisXVerts, head.numVisZVerts);
               }

               using (PerfSection p2 = new PerfSection("write verts, alphas, and AO"))
               {

                  for (int i = 0; i < numVerts; i++)
                  {
                     f.Write(float.IsNaN(detailPoints[i].X) ? 0 : detailPoints[i].X);
                     f.Write(float.IsNaN(detailPoints[i].Y) ? 0 : detailPoints[i].Y);
                     f.Write(float.IsNaN(detailPoints[i].Z) ? 0 : detailPoints[i].Z);
                  }
                  for (int i = 0; i < numVerts; i++)
                  {
                     f.Write(alphaVals[i]);
                  }
                  for (int i = 0; i < numVerts; i++)
                  {
                     f.Write(aoVals[i]);
                  }
               }
               int ival;
               float fval;
               byte bval;
               using (PerfSection p3 = new PerfSection("write sim data"))
               {
                 
                  JaggedContainer<int> pass = TerrainGlobals.getEditor().getSimRep().getDataTiles().getJaggedPassable();
                  pass.SaveByStripe(f,
                     (JaggedContainer<int>.SaveStripeDelegate)(delegate(BinaryWriter w, int[] values)
                     {
                        for (int i = 0; i < values.Length; i++)
                        {
                           w.Write(values[i]);
                        }
                     }));

                  
                  JaggedContainer<float> height = TerrainGlobals.getEditor().getSimRep().getHeightRep().getJaggedHeight();
                  height.SaveByStripe(f,
                     (JaggedContainer<float>.SaveStripeDelegate)(delegate(BinaryWriter w, float[] values)
                     {
                        for (int i = 0; i < values.Length; i++)
                        {
                           w.Write(values[i]);
                        }
                     }));


                  JaggedContainer<byte> tess = TerrainGlobals.getEditor().getJaggedTesselation();
                  

                  tess.SaveByStripe(f,
                     (JaggedContainer<byte>.SaveStripeDelegate)(delegate(BinaryWriter w, byte[] values)
                     {
                        w.Write(values);
                     }));



                  //write our camera width and height values
                  f.Write(TerrainGlobals.getEditor().getCameraRep().getNumXPoints());
                  f.Write(TerrainGlobals.getEditor().getCameraRep().getNumZPoints());
                  JaggedContainer<float> camHeight = TerrainGlobals.getEditor().getCameraRep().getJaggedHeight();

                  camHeight.SaveByStripe(f,
                    (JaggedContainer<float>.SaveStripeDelegate)(delegate(BinaryWriter w, float[] values)
                     {
                        for (int i = 0; i < values.Length; i++)
                        {
                           w.Write(values[i]);
                        }
                     }));


                  //flight rep
                  f.Write(TerrainGlobals.getEditor().getSimRep().getFlightRep().getHeightGenOffset());
                  JaggedContainer<float> flightHeight = TerrainGlobals.getEditor().getSimRep().getFlightRep().getJaggedHeight();

                  flightHeight.SaveByStripe(f,
                    (JaggedContainer<float>.SaveStripeDelegate)(delegate(BinaryWriter w, float[] values)
                     {
                        for (int i = 0; i < values.Length; i++)
                        {
                           w.Write(values[i]);
                        }
                     }));

                  //write sim buildable
                  JaggedContainer<int> build = TerrainGlobals.getEditor().getSimRep().getDataTiles().getJaggedBuildable();
                  build.SaveByStripe(f,
                     (JaggedContainer<int>.SaveStripeDelegate)(delegate(BinaryWriter w, int[] values)
                     {
                        for (int i = 0; i < values.Length; i++)
                        {
                           w.Write(values[i]);
                        }
                     }));

                  //write sim flood passable
                  JaggedContainer<int> floodpass = TerrainGlobals.getEditor().getSimRep().getDataTiles().getJaggedFloodPassable();
                  floodpass.SaveByStripe(f,
                     (JaggedContainer<int>.SaveStripeDelegate)(delegate(BinaryWriter w, int[] values)
                     {
                        for (int i = 0; i < values.Length; i++)
                        {
                           w.Write(values[i]);
                        }
                     }));

                  //write sim scarab passable
                  JaggedContainer<int> scarabpass = TerrainGlobals.getEditor().getSimRep().getDataTiles().getJaggedScarabPassable();
                  scarabpass.SaveByStripe(f,
                     (JaggedContainer<int>.SaveStripeDelegate)(delegate(BinaryWriter w, int[] values)
                     {
                        for (int i = 0; i < values.Length; i++)
                        {
                           w.Write(values[i]);
                        }
                     }));

                  //write Tile type
                  JaggedContainer<int> ttype = TerrainGlobals.getEditor().getSimRep().getDataTiles().getJaggedTileType();
                  ttype.SaveByStripe(f,
                     (JaggedContainer<int>.SaveStripeDelegate)(delegate(BinaryWriter w, int[] values)
                     {
                        for (int i = 0; i < values.Length; i++)
                        {
                           w.Write(values[i]);
                        }
                     }));

               }

               using (PerfSection p4 = new PerfSection("preSaveToTED()"))
               {
                  //DO SOME CLEANUP HERE!!
                  TerrainGlobals.getTexturing().preSaveToTED();
               }

               using (PerfSection p5 = new PerfSection("write out texture information"))
               {
                  // write out texture information
                  BTerrainQuadNode[] nodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();

                  int numXChunkNodes = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.getMaxNodeWidth());
                  for (int i = 0; i < nodes.Length; i++)
                  {
                     //WRITE OUR OUR INDEX POSITION FIRST!!!
                     BTerrainQuadNodeDesc desc = nodes[i].getDesc();
                     int minx = (int)(desc.mMinXVert / BTerrainQuadNode.getMaxNodeWidth());
                     int minz = (int)(desc.mMinZVert / BTerrainQuadNode.getMaxNodeWidth());

                     //we write these to file to use as an indexing param for our loading..
                     f.Write(minx);
                     f.Write(minz);

                     //write the number of layers
                     int numLayers = nodes[i].mLayerContainer.getNumLayers();
                     f.Write(numLayers);
                     for (int k = 0; k < numLayers; k++)
                     {
                        BTerrainTexturingLayer layer = nodes[i].mLayerContainer.giveLayer(k);
                        f.Write((int)layer.mLayerType);
                        f.Write(layer.mActiveTextureIndex);
                        f.Write(layer.mAlphaLayer);
                     }
                  }
               }

               // Save out texture names
               f.Write(SimTerrainType.mActiveWorkingSet.Count);
               for (int i = 0; i < SimTerrainType.mActiveWorkingSet.Count; i++)
               {
                  f.Write(SimTerrainType.getFromNumber(SimTerrainType.mActiveWorkingSet[i]).TypeName);

                  //visual properties for this map
                  BTerrainActiveTextureContainer act = TerrainGlobals.getTexturing().getActiveTexture(i);
                  if (act != null)
                  {
                     f.Write(act.mUScale);
                     f.Write(act.mVScale);
                     f.Write(act.mBlendOp);
                     f.Write(act.mSpecExponent);
                     f.Write(act.mFresnelBias);
                     f.Write(act.mFresnelPower);
                     f.Write(act.mFresnelScale);
                     f.Write(act.mFresnelRefractPercent);
                  }
                  else
                  {
                     act = TerrainGlobals.getTexturing().getActiveTexture(0);
                     f.Write(act.mUScale);
                     f.Write(act.mVScale);
                     f.Write(act.mBlendOp);
                     f.Write(act.mSpecExponent);
                     f.Write(act.mFresnelBias);
                     f.Write(act.mFresnelPower);
                     f.Write(act.mFresnelScale);
                     f.Write(act.mFresnelRefractPercent);
                  }
               }

               // Save out our decals
               f.Write(TerrainGlobals.getTexturing().getActiveDecalCount());
               for (int i = 0; i < TerrainGlobals.getTexturing().getActiveDecalCount(); i++)
               {
                  BTerrainActiveDecalContainer decal = TerrainGlobals.getTexturing().getActiveDecal(i);
                  f.Write(decal.mFilename);
               }

               // Save out our decal instances
               f.Write(TerrainGlobals.getTexturing().getActiveDecalInstancesCount());
               for (int i = 0; i < TerrainGlobals.getTexturing().getActiveDecalInstancesCount(); i++)
               {
                  BTerrainDecalInstance decalInst = TerrainGlobals.getTexturing().getActiveDecalInstance(i);
                  f.Write(decalInst.mActiveDecalIndex);
                  f.Write(decalInst.mRotation);
                  f.Write(decalInst.mTileBoundsMax.X);
                  f.Write(decalInst.mTileBoundsMax.Y);
                  f.Write(decalInst.mTileBoundsMin.X);
                  f.Write(decalInst.mTileBoundsMin.Y);
                  f.Write(decalInst.mTileCenter.X);
                  f.Write(decalInst.mTileCenter.Y);
                  f.Write(decalInst.mUScale);
                  f.Write(decalInst.mVScale);
               }

               //ROADS
               f.Write(RoadManager.giveNumRoads());
               for (int i = 0; i < RoadManager.giveNumRoads(); i++)
               {
                  Road rd = RoadManager.giveRoad(i);

                  if (rd.getNumControlPoints() == 0)  //don't export roads w/o points on the map
                     continue;

                  f.Write(rd.getRoadTextureName());
                  f.Write(rd.getRoadWidth());
                  f.Write(rd.getWorldTesselationSize());

                  f.Write(rd.getNumControlPoints());
                  for (int k = 0; k < rd.getNumControlPoints(); k++)
                  {
                     int type = (int)rd.getPoint(k).mControlPointType;
                     f.Write(type);
                     f.Write(rd.getPoint(k).mGridX);
                     f.Write(rd.getPoint(k).mGridY);
                  }

                  f.Write(rd.getNumRoadSegments());
                  for (int k = 0; k < rd.getNumRoadSegments(); k++)
                  {
                     roadSegment rs = rd.getRoadSegment(k);
                     f.Write(rs.mPointA.mGridX);
                     f.Write(rs.mPointA.mGridY);
                     f.Write(rs.mPointB.mGridX);
                     f.Write(rs.mPointB.mGridY);
                  }
               }


               using (PerfSection p9 = new PerfSection("FOLIAGE"))
               {

                  ///FOLIAGE!
                  f.Write(FoliageManager.getNumSets());
                  for (int i = 0; i < FoliageManager.getNumSets(); i++)
                  {
                     FoliageSet fs = FoliageManager.giveSet(i);
                     f.Write(Path.GetFileNameWithoutExtension(fs.mFullFileName));
                  }
                  JaggedContainer<FoliageVertData> foliage = FoliageManager.mVertData;
                  foliage.SaveByStripe(f,
                     (JaggedContainer<FoliageVertData>.SaveStripeDelegate)(delegate(BinaryWriter w, FoliageVertData[] values)
                     {
                        for (int i = 0; i < values.Length; i++)
                        {
                           f.Write(FoliageManager.giveIndexOfSet(values[i].mFoliageSetName));
                           f.Write(values[i].mFoliageSetBladeIndex);
                        }
                     }));

               }
               using (PerfSection p10 = new PerfSection("flush time"))
               {
                  if (bs != null)
                  {
                     bs.Flush();
                  }
                  //s.Write(ms.GetBuffer(), 0, (int)ms.Length);
               }
               f.Close();
               s.Close();
            }         
            catch(System.Exception ex)
            {
               CoreGlobals.ShowMessage("FatalError saving file.  A backup of the last good save was made as " + filename + ".backup");
               CoreGlobals.FatalSaveError = true;
               //CoreGlobals.getErrorManager().LogErrorToNetwork(ex.ToString());
               throw ex;
            }
         }
      }

      static private void sticthEdgesOfMap(ref Vector3[] positions, int numXVerts, int numZVerts)
      {
         // Fixed edges to not have any displacment along the X-Z plane
         int index;
         int x, z;
         for (int i = 0; i < numZVerts; i++)
         {
            // X = 0
            x = 0;
            z = i;
            index = x * numZVerts + z;
            positions[index].X = 0.0f;

            // X = numXVerts - 1
            x = numXVerts - 1;
            z = i;
            index = x * numZVerts + z;
            positions[index].X = 0.0f;
         }

         for (int i = 0; i < numXVerts; i++)
         {
            // Z = 0
            x = i;
            z = 0;
            index = x * numZVerts + z;
            positions[index].Z = 0.0f;

            // X = numZVerts - 1
            x = i;
            z = numZVerts - 1;
            index = x * numZVerts + z;
            positions[index].Z = 0.0f;
         }
      }

    
      
      //----------------------------------------------
      static public bool TEDto360(string terrainFileName, string scenarioFileName,
                                 string outputOverrideDir,
                                  Export360.ExportSettings expSettings,
                                  bool doXTD, bool doXTT, bool doXTH, bool doXSD, bool doLRP, bool doDEP, bool doTAG, bool doXMB)
      {
         CoreGlobals.IsBatchExport = true;

         if (expSettings == null)
            expSettings = TerrainGlobals.getTerrain().getExportSettings();

         TerrainGlobals.getTerrainFrontEnd().LoadProject(Path.ChangeExtension(scenarioFileName, ".SCN"), false);//!expSettings.ObjectsInAO);

         TerrainGlobals.getTexturing().destroyCaches();  //clear out some memory that we need...

         

         Export360.ExportResults results = new Export360.ExportResults();
         bool exportOK = ExportTo360.doExport(Path.ChangeExtension(terrainFileName, ".XTD"),
                                             Path.ChangeExtension(scenarioFileName, ".XSD"),
                                             outputOverrideDir,
                                             ref expSettings, ref results,
                                             doXTD, doXTT, doXTH, doXSD, doLRP, doDEP, doTAG, doXMB);

         CoreGlobals.IsBatchExport = false;
         

             
         return exportOK;
      }
      static public bool TEDtoAOSection   (string terrainFileName, string scenarioFileName,
                                          string outputOverrideDir,
                                          int numSections, int mySection, 
                                          Export360.ExportSettings expSettings)
      {
         CoreGlobals.IsBatchExport = true;

         if (expSettings == null)
            expSettings = TerrainGlobals.getTerrain().getExportSettings();

         TerrainGlobals.getTerrainFrontEnd().LoadProject(Path.ChangeExtension(scenarioFileName, ".SCN"), false);

         TerrainGlobals.getTexturing().destroyCaches();  //clear out some memory that we need...


         //work\scenario\development\coltTest\coltTest.AO0, .AO1, .AO2...., .AON
         string outFileName = Path.ChangeExtension(terrainFileName, ".AO" + mySection);

         NetworkedAOGen aoGen = new NetworkedAOGen();
         aoGen.computeAOSection(outFileName, numSections, mySection, expSettings.AmbientOcclusion);

    
         CoreGlobals.IsBatchExport = false;

         return true;
      }


   }

   #endregion
}
