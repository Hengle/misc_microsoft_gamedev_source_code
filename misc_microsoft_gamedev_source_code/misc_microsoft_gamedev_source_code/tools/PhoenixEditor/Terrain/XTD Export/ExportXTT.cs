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
using Terrain;
using Rendering;

namespace Export360
{
     //--------------------------------------------
   public class XTTExporter
   {
      ~XTTExporter()
      {
         destroy();
      }

      public void destroy()
      {
         clean();
         mChunkTextureList = null;
      }

      private string giveLocalTexName(String inTexName)
      {
         string fname = inTexName;
         int k = inTexName.LastIndexOf(@"terrain") + 8;
         fname = fname.Substring(k, inTexName.Length - k);
         fname = fname.Substring(0, fname.LastIndexOf(@"_df"));

         return fname;
      }

      private char []convTexName(string fname)
      {
         char[] filename = new char[256];
         fname.CopyTo(0, filename, 0, fname.Length);
         return filename;
      }
      private unsafe bool writeHeader()
      {

         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         binWriter.Write(Xbox_EndianSwap.endSwapI32((int)eFileVersions.cXTTVersion));

         int zero = 0;
         //counts for each type
         binWriter.Write(Xbox_EndianSwap.endSwapI32(TerrainGlobals.getTexturing().getActiveTextureCount()));
         if (TerrainGlobals.getTexturing().getActiveDecalCount()!=0)
         {
            binWriter.Write(Xbox_EndianSwap.endSwapI32(TerrainGlobals.getTexturing().getActiveDecalCount()));
            binWriter.Write(Xbox_EndianSwap.endSwapI32(TerrainGlobals.getTexturing().getActiveDecalInstancesCount()));
         }
         else
         {
            binWriter.Write(zero);
            binWriter.Write(zero);
         }
          

         //active textures
         for (int i = 0; i < TerrainGlobals.getTexturing().getActiveTextureCount(); i++)
         {
            String texName = giveLocalTexName(TerrainGlobals.getTexturing().getActiveTexture(i).mFilename);
            if(texName.Length>=256)
            {
               MessageBox.Show("ERROR! " + texName + " is longer than 256 characters. Please rename it. \n We're working on a fix for this..");
               binWriter.Close();
               binWriter = null;
               chunkHolder.Close();
               chunkHolder = null;
               return false ;
            }
            binWriter.Write(convTexName(texName));

            ExportTo360.addTextureChannelDependencies(TerrainGlobals.getTexturing().getActiveTexture(i).mFilename);

            //WRITE OUR LAYER INFO
            binWriter.Write(Xbox_EndianSwap.endSwapI32(TerrainGlobals.getTexturing().getActiveTexture(i).mUScale));     //mUScale
            binWriter.Write(Xbox_EndianSwap.endSwapI32(TerrainGlobals.getTexturing().getActiveTexture(i).mVScale));     //mVScale
            binWriter.Write(Xbox_EndianSwap.endSwapI32(TerrainGlobals.getTexturing().getActiveTexture(i).mBlendOp));     //mBlendOp
         }

         if (TerrainGlobals.getTexturing().getActiveDecalCount() != 0)
         {
            //decal textures
            for (int i = 0; i < TerrainGlobals.getTexturing().getActiveDecalCount(); i++)
            {
               String texName = giveLocalTexName(TerrainGlobals.getTexturing().getActiveDecal(i).mFilename);
               binWriter.Write(convTexName(texName));
               ExportTo360.addTextureChannelDependencies(TerrainGlobals.getTexturing().getActiveDecal(i).mFilename);
            }


            //decal instances
            for (int i = 0; i < TerrainGlobals.getTexturing().getActiveDecalInstancesCount(); i++)
            {
               binWriter.Write(Xbox_EndianSwap.endSwapI32(TerrainGlobals.getTexturing().getActiveDecalInstance(i).mActiveDecalIndex));
               binWriter.Write(Xbox_EndianSwap.endSwapF32(TerrainGlobals.getTexturing().getActiveDecalInstance(i).mRotation));
               binWriter.Write(Xbox_EndianSwap.endSwapF32(TerrainGlobals.getTexturing().getActiveDecalInstance(i).mTileCenter.X));
               binWriter.Write(Xbox_EndianSwap.endSwapF32(TerrainGlobals.getTexturing().getActiveDecalInstance(i).mTileCenter.Y));
               binWriter.Write(Xbox_EndianSwap.endSwapF32(TerrainGlobals.getTexturing().getActiveDecalInstance(i).mUScale));
               binWriter.Write(Xbox_EndianSwap.endSwapF32(TerrainGlobals.getTexturing().getActiveDecalInstance(i).mVScale));
            }
         }


         ExportTo360.mECF.addChunk((int)eXTT_ChunkID.cXTT_XTTHeader, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;
         return true;
      }

      
      public bool export_XTT(string filename, ExportSettings expSettings, ref ExportResults results)
      {
         TerrainGlobals.getTexturing().preExportTo360();

         if (!writeHeader())
            return false;
         writeTextureChunks(ref results);
         generateUniqueAtlas(ref expSettings, ref results);

         XTT_RoadExport mXTTRoadExport = new XTT_RoadExport();
         mXTTRoadExport.exportRoads(ref results);
         mXTTRoadExport = null;

         XTT_FoliageExport mXTTFoliageExport = new XTT_FoliageExport();
         mXTTFoliageExport.exportFoliage(ref results);
         mXTTFoliageExport = null;

         bool exportOK = ExportTo360.safeECFFileWrite(filename, ".XTT");

         clean();
         return exportOK;
      }



      #region COMPOSITE ATLAS
      public void toTextureArray(ref List<Texture> mTempAlphaTextures, XTDTextureLinkerData linkerDat)
      {
         //lock in our alpha texture
         int slicePitch = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());
         int count = Math.Min(mTempAlphaTextures.Count, linkerDat.layerData.getNumLayers());

         int i = 0;
         for (i = 0; i < count; i++)
         {
            if (mTempAlphaTextures[i] != null)
            {
               GraphicsStream texstream = mTempAlphaTextures[i].LockRectangle(0, LockFlags.None);
               //CLM this should already be done for us
               //byte[] bordered = XTD_CompressTextures.gutterAlphaTextureForBlending(linkerDat.layerData.giveLayer(i), linkerDat);

               texstream.Write(linkerDat.layerData.giveLayer(i).mAlphaLayer, 0, slicePitch);
               mTempAlphaTextures[i].UnlockRectangle(0);
               //  bordered = null;
            }
            else
            {

            }
         }

         //we've got more layers than we've preallocated
         if (mTempAlphaTextures.Count < linkerDat.layerData.getNumLayers())
         {
            int diff = linkerDat.layerData.getNumLayers() - mTempAlphaTextures.Count;
            for (int k = 0; k < diff; k++)
            {
               mTempAlphaTextures.Add(new Texture(BRenderDevice.getDevice(), (int)BTerrainTexturing.getAlphaTextureWidth(), (int)BTerrainTexturing.getAlphaTextureHeight(), 1, 0, Format.L8, Pool.Managed));

               GraphicsStream texstream = mTempAlphaTextures[mTempAlphaTextures.Count - 1].LockRectangle(0, LockFlags.None);
               //CLM this should already be done for us
               //byte[] bordered = XTD_CompressTextures.gutterAlphaTextureForBlending(linkerDat.layerData.giveLayer(i + k),linkerDat);

               texstream.Write(linkerDat.layerData.giveLayer(i + k).mAlphaLayer, 0, slicePitch);
               mTempAlphaTextures[mTempAlphaTextures.Count - 1].UnlockRectangle(0);
               //bordered = null;


            }
         }
      }
      public unsafe void generateUniqueAtlas(ref ExportSettings settings, ref ExportResults results)
      {
         DateTime n = DateTime.Now;
         //step 1, ensure all our textures are loaded, and in the proper active texture indexes
         for (int i = 0; i < TerrainGlobals.getTexturing().getActiveTextureCount(); i++)
         {
            //is this texture loaded in our texture cache?
            int l = TerrainGlobals.getTexturing().addActiveTexture(TerrainGlobals.getTexturing().getActiveTexture(i).mFilename);
            if (l != i)
            {
               Debug.Assert(true);//SHIT !
            }
         }

         if (TerrainGlobals.getTexturing().getActiveDecalCount() != 0)
         {
            for (int i = 0; i < TerrainGlobals.getTexturing().getActiveDecalCount(); i++)
            {
               //is this texture loaded in our texture cache?
               int l = TerrainGlobals.getTexturing().addActiveDecal(TerrainGlobals.getTexturing().getActiveDecal(i).mFilename);
               if (l != i)
               {
                  Debug.Assert(true);//SHIT !
               }
            }
         }

         //////////////////////////////////////////////////////////////////////////
         //////////////////////////////////////////////////////////////////////////

         //step 2, setup the texturcompistor data
         if (TerrainGlobals.getTexturing().mCompositeShader == null)
         {
            TerrainGlobals.getTexturing().initComposite();
         }

         int lodLevel = settings.UniqueTexRes;// 2; //=128x128
         int subWidth = BTerrainTexturing.LODToWidth(lodLevel);
         int numXChunk = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.getMaxNodeWidth());
         int numRows = numXChunk;
         int numCols = numXChunk;
         int uniqueWidth = subWidth * numRows;
         int uniqueHeight = subWidth * numCols;

         BTerrainCompositeAtlasTexture atlas = new BTerrainCompositeAtlasTexture(subWidth, subWidth, numRows * numCols, numRows, numCols, 1, Format.A8R8G8B8);


         //////////////////////////////////////////////////////////////////////////
         //////////////////////////////////////////////////////////////////////////
         //step 3, for each BTerrainTextureContainer, composite to a section of the atlas.
         Surface biggerDepthStencil = BRenderDevice.getDevice().CreateDepthStencilSurface(uniqueWidth, uniqueHeight, DepthFormat.D24S8, MultiSampleType.None, 0, false);
         
         List<Texture> tempAlphaList = new List<Texture>();


         BRenderDevice.getDevice().BeginScene();
         TerrainGlobals.getTexturing().preConvertSetup();
          BRenderDevice.getDevice().DepthStencilSurface = biggerDepthStencil;
         for (int i = 0; i < mChunkTextureList.Count; i++)
         {
            int atlasIndex = mChunkTextureList[i].gridLocY + mChunkTextureList[i].gridLocX * numXChunk;
            BTerrainCompositeTexture ouput = new BTerrainCompositeTexture(subWidth, subWidth, 1, atlas, atlasIndex);

            //create a temp alpha tex array for this

            int minXVert = mChunkTextureList[i].gridLocX * (int)BTerrainQuadNode.cMaxWidth;
            int minZVert = mChunkTextureList[i].gridLocY * (int)BTerrainQuadNode.cMaxWidth;


            toTextureArray(ref tempAlphaList, mChunkTextureList[i]);

            TerrainGlobals.getTexturing().convertLayersToTexturingDataHandle(mChunkTextureList[i].layerData,
                                                                             tempAlphaList,
                                                                             ouput, minXVert, minZVert, lodLevel,
                                                                             1);

            ouput.destroy();
            ouput = null;

            //  BRenderDevice.writeTextureToFile(atlas.mTextures[(int)BTerrainTexturing.eTextureChannels.cAlbedo], AppDomain.CurrentDomain.BaseDirectory + "outAtlas.bmp");

         }
         TerrainGlobals.getTexturing().postConvertSetup();
         BRenderDevice.getDevice().EndScene();

         biggerDepthStencil.Dispose();
         biggerDepthStencil = null;

         for (int i = 0; i < tempAlphaList.Count; i++)
            tempAlphaList[i].Dispose();
         tempAlphaList.Clear();

         ////////////////////////////////////////////////////////////////////////
         ////////////////////////////////////////////////////////////////////////
         //step 4, compress, and write to our ecf writer

         //copy our rendertarget to a usable surface
         Surface atlasSurf = atlas.mTextures[(int)BTerrainTexturing.eTextureChannels.cAlbedo].GetSurfaceLevel(0);
         Texture albedo = new Texture(BRenderDevice.getDevice(), uniqueWidth, uniqueHeight, 1, 0, Format.A8R8G8B8, Pool.SystemMemory);
         Surface albedoSurf = albedo.GetSurfaceLevel(0);
         BRenderDevice.getDevice().GetRenderTargetData(atlasSurf, albedoSurf);

         //copy our data from our composited texture
         byte[] inData = new byte[uniqueHeight * uniqueWidth * 4];
         GraphicsStream texstream = albedo.LockRectangle(0, LockFlags.None);
         byte* aDat = (byte*)texstream.InternalDataPointer;

         //for (int x = 0; x < uniqueWidth * uniqueHeight * 4; x++)
         //   inData[x] = aDat[x];

         for (int x = 0; x < uniqueWidth; x++)
         {
            for (int z = 0; z < uniqueHeight; z++)
            {
               int dstIndex = 4 * (x * uniqueWidth + z);
               int srcIndex = 4 * (x + uniqueWidth * z);

               inData[dstIndex + 0] = aDat[srcIndex + 2];
               inData[dstIndex + 1] = aDat[srcIndex + 1];
               inData[dstIndex + 2] = aDat[srcIndex + 0];
               inData[dstIndex + 3] = 255;// aDat[srcIndex + 3];
            }
         }

         albedo.UnlockRectangle(0);

         


         //MIP0
         byte[] mip0Dat = null;
         int outMemSize = 0;
         {
            //compress it
            ExportTo360.toCompressedFormat(inData, uniqueWidth, uniqueHeight, sizeof(UInt32), BDXTFormat.cDXT1, settings.ExportTextureCompressionQuality, ref mip0Dat, ref outMemSize);

            //endianswap this data...
            int count = outMemSize;
            for (int i = 0; i < count - 1; i += 2)
            {
               byte a = mip0Dat[i];
               mip0Dat[i] = mip0Dat[i + 1];
               mip0Dat[i + 1] = a;
            }

            //tileswap
            byte[] tempbytearray = new byte[mip0Dat.Length];
            mip0Dat.CopyTo(tempbytearray, 0);
            ExportTo360.tileCopy(ref tempbytearray, tempbytearray, (int)uniqueWidth, (int)uniqueHeight, (int)ExportTo360.eTileCopyFormats.cTCFMT_DXT1);
            tempbytearray.CopyTo(mip0Dat, 0);
         }

         //MIPS!!
         int numMips = BMathLib.isPow2 (uniqueWidth) ? 1 : 0;
         byte[] mip1Dat = null;

         if(numMips!=0)
         {
            int oMS = outMemSize;
            for (int i = 0; i < numMips; i++)
            {
               outMemSize += oMS >> 2;
               oMS = oMS >> 1;
            }
            int outMemSizeMip1 = 0;
            int mipWidth = uniqueWidth;
            int mipHeight = uniqueHeight;
            {
               //resize
               mipWidth = mipWidth >> 1;
               mipHeight = mipHeight >> 1;

               byte[] mipData = null;
               ImageManipulation.resizeRGBATexture(inData, uniqueWidth, uniqueHeight, ref mipData, mipWidth, mipHeight, ImageManipulation.eFilterType.cFilter_Linear);

               //compress it
               ExportTo360.toCompressedFormat(mipData, mipWidth, mipHeight, sizeof(UInt32), BDXTFormat.cDXT1, settings.ExportTextureCompressionQuality, ref mip1Dat, ref outMemSizeMip1);

               //endianswap this data...
               int count = outMemSizeMip1;
               for (int i = 0; i < count - 1; i += 2)
               {
                  byte a = mip1Dat[i];
                  mip1Dat[i] = mip1Dat[i + 1];
                  mip1Dat[i + 1] = a;
               }

               //tileswap
               byte[] tempbytearray = new byte[mip1Dat.Length];
               mip1Dat.CopyTo(tempbytearray, 0);
               ExportTo360.tileCopy(ref tempbytearray, tempbytearray, (int)mipWidth, (int)mipHeight, (int)ExportTo360.eTileCopyFormats.cTCFMT_DXT1);
               tempbytearray.CopyTo(mip1Dat, 0);

            }
         }
       
        
         //write to ecf
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         binWriter.Write(Xbox_EndianSwap.endSwapI32(outMemSize));
         binWriter.Write(Xbox_EndianSwap.endSwapI32(uniqueWidth));
         binWriter.Write(Xbox_EndianSwap.endSwapI32(uniqueHeight));
         binWriter.Write(Xbox_EndianSwap.endSwapI32(numMips+1));
         binWriter.Write(mip0Dat);
         if (mip1Dat!=null)
            binWriter.Write(mip1Dat);

         ExportTo360.mECF.addChunk((int)eXTT_ChunkID.cXTT_AtlasChunkAlbedo, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;


         results.terrainUniqueTextureMemorySize += outMemSize;
         //////////////////////////////////////////////////////////////////////////
         //////////////////////////////////////////////////////////////////////////
         //step 5, if we're batch exporting, clear our active texutre list.

         albedo.Dispose();
         albedo = null;
         albedoSurf.Dispose();
         atlasSurf.Dispose();

         atlas.destroy();
         atlas = null;

         TimeSpan ts = DateTime.Now - n;
         results.terrainUniqueTextureTime = ts.TotalMinutes;
      }
      
     
      #endregion

      #region COMPRESSTEXTURES
      [StructLayout(LayoutKind.Sequential)]
      public unsafe class XTDTextureLinkerData
      {
         ~XTDTextureLinkerData()
         {
            if (layerData != null)
            {
               layerData.destroy();
               layerData = null;
            }
            mOwnerNode = null;
         }
         public int gridLocX;
         public int gridLocY;

         public BTerrainLayerContainer layerData = null;

         public BTerrainQuadNode mOwnerNode = null;
      }

      public List<XTDTextureLinkerData> mChunkTextureList = new List<XTDTextureLinkerData>();


      public void clean()
      {
         if (mChunkTextureList!=null)
         {
            for (int i = 0; i < mChunkTextureList.Count; i++)
               mChunkTextureList[i] = null;

            mChunkTextureList.Clear();
         }
         
      }





      //-----------------------------
      //-----------------------------
      //-----------------------------
      //-----------------------------

      //private unsafe void removeHiddenLayers(ref BTerrainLayerContainer texDat)
      //{
      //   int sze = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());

      //   //remove any layers that are hidden
      //   int[] visibleAlpha = new int[sze];
      //   for (int i = 1; i < texDat.getNumLayers(); i++)
      //   {
      //      //DECALS CANNOT HIDE OTHERS
      //      if (texDat.giveLayer(i).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
      //         continue;

      //      texDat.giveLayer(i).mAlphaLayer.CopyTo(visibleAlpha, 0);

      //      //remove layers that have no visible contribution after flattening
      //      for (int t = i + 1; t < texDat.getNumLayers(); t++)
      //      {
      //         if (texDat.giveLayer(t).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
      //            continue;

      //         for (int x = 0; x < sze; x++)
      //         {
      //            visibleAlpha[x] -= texDat.giveLayer(t).mAlphaLayer[x];
      //         }
      //      }

      //      bool removeMe = true;
      //      int numVisPixels = 0;
      //      for (int x = 0; x < sze; x++)
      //      {
      //         if (visibleAlpha[x] > 0)
      //         {
      //            numVisPixels++;
      //          //  if (numVisPixels > sze * 0.05f)
      //            {
      //               removeMe = false;
      //               break;
      //            }
      //         }
      //      }
      //      if (removeMe)
      //      {
      //         texDat.removeLayer(i);
      //         i--;
      //      }

      //   }

      //   visibleAlpha = null;
      //}
      //private unsafe void removeRepeditiveLayers(ref BTerrainLayerContainer texDat)
      //{
      //   int sze = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());

      //   //remove repeditive layers
      //   for (int i = 0; i < texDat.getNumLayers(); i++)
      //   {
      //      //find another layer with my same ID;
      //      int idToFind = texDat.giveLayer(i).mActiveTextureIndex;

      //      BTerrainTexturingLayer.eLayerType type = texDat.giveLayer(i).mLayerType;
      //      if (type == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
      //         continue;

      //      for (int q = i + 1; q < texDat.getNumLayers(); q++)
      //      {
      //         if (texDat.giveLayer(q).mActiveTextureIndex == idToFind && texDat.giveLayer(q).mLayerType == type)
      //         {
      //            //eliminate all pixels in all layers between the two of us
      //            //by our top layer 
      //            for (int ind = i + 1; ind < q; ind++)
      //            {
      //               for (int k = 0; k < sze; k++)
      //               {
      //                  if (texDat.giveLayer(q).mAlphaLayer[k] != 0)
      //                  {
      //                     int v = texDat.giveLayer(ind).mAlphaLayer[k] - texDat.giveLayer(q).mAlphaLayer[k];

      //                     if (v > Byte.MaxValue)
      //                        v = Byte.MaxValue;

      //                     if(v<=0)
      //                        texDat.giveLayer(ind).mAlphaLayer[k] = 0;
      //                     else
      //                        texDat.giveLayer(ind).mAlphaLayer[k] = ((Byte)v);
      //                  }
      //               }
      //            }

      //            //add this layer's values to our own
      //            for (int k = 0; k < sze; k++)
      //            {
      //               if (texDat.giveLayer(q).mAlphaLayer[k] != 0)
      //               {
      //                  texDat.giveLayer(i).mAlphaLayer[k] = (byte)(BMathLib.Clamp(texDat.giveLayer(i).mAlphaLayer[k] + texDat.giveLayer(q).mAlphaLayer[k], 0, 255));
      //               }
      //            }

      //            texDat.removeLayer(q);

      //            i--;
      //            break;
      //         }
      //      }
      //   }

      //}
      //private unsafe void flattenLayers(ref BTerrainLayerContainer texDat)
      //{
      //   int sze = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());

      //   //easy out flattening
      //   for (int i = 0; i < texDat.mTextures.Count; i++)
      //   {
      //      //DECALS CANNOT HIDE OTHERS
      //      if (texDat.mTextures[i].mLayerType == (int)BTerrainTexturingLayer.eLayerType.cLayer_Decal)
      //         continue;

      //      bool removeMe = true;
      //      for (int x = 0; x < sze; x++)
      //      {
      //         if (texDat.mTextures[i].mAlphaTexture[x] != byte.MaxValue)
      //         {
      //            removeMe = false;
      //            break;
      //         }
      //      }
      //      if (removeMe)
      //      {
      //         if (i == 0)
      //            continue;

      //         texDat.mTextures.RemoveRange(0, i);
      //         texDat.mNumSplatLayers -= i;

      //         for (int x = 0; x < sze; x++)
      //         {
      //            texDat.mTextures[0].mAlphaLayer[x] = Byte.MaxValue;
      //         }
      //         i = 0;

      //      }
      //   }

      //}
      // private unsafe void reCalcAlphaToContrib(ref BTerrainLayerContainer texDat)
      //{
      //    int sze = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());

      //    for (int i = 0; i < texDat.mTextures.Count; i++)
      //    {
      //       for (int x = 0; x < sze; x++)
      //       {
      //          Byte contrib = texDat.mTextures[i].mAlphaTexture[x];
      //          //find the final contribution of this pixel after all the alpha blending operations have occured
      //          for (int k = i+1; k < texDat.mTextures.Count; k++)
      //          {
      //             contrib -= texDat.mTextures[k].mAlphaTexture[x];
      //          }
      //          if (contrib < 0)  contrib = 0;
      //          texDat.mTextures[i].mAlphaTexture[x] = contrib;
      //       }
      //    }
      //}

      private unsafe void ensureDecalAlphas(ref BTerrainLayerContainer texDat)
      {
         int sze = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());

         //find a decal. Walk upwards and subtract out all influences to us.
         for (int i = 0; i < texDat.getNumLayers(); i++)
         {
            if (texDat.giveLayer(i).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
            {
               //walk upwards
               for (int t = i + 1; t < texDat.getNumLayers(); t++)
               {
                  if (texDat.giveLayer(t).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
                  {
                     //subtract from our alpha
                     for (int x = 0; x < sze; x++)
                     {
                        texDat.giveLayer(i).mAlphaLayer[x] = (byte)(BMathLib.Clamp(texDat.giveLayer(i).mAlphaLayer[x] - texDat.giveLayer(t).mAlphaLayer[x], 0, 255));
                     }
                  }
               }
            }
         }
      }
      private unsafe void removeBlankLayers(ref BTerrainLayerContainer texDat)
      {
         int sze = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());

         for (int i = 1; i < texDat.getNumLayers(); i++)
         {
            //DECALS CANNOT HIDE OTHERS
            if (texDat.giveLayer(i).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
               continue;

            bool blank = true;
            for (int x = 0; x < sze; x++)
            {
               if (texDat.giveLayer(i).mAlphaLayer[x] != byte.MinValue)
               {
                  blank = false;
                  break;
               }
            }

            if (blank)
            {
               texDat.removeLayer(i);
               i--;
            }

         }
      }
      private unsafe void cleanupLayerData(ref BTerrainLayerContainer texDat)
      {
         ensureDecalAlphas(ref texDat);
         removeBlankLayers(ref texDat); //this is super important to our 360 memory footprint
         //     removeRepeditiveLayers(ref texDat);
         //     removeHiddenLayers(ref texDat);
         //     flattenLayers(ref texDat);

         // reCalcAlphaToContrib(ref texDat);
      }

      public unsafe void addTextureChunk(int gridLocX, int gridLocY, BTerrainLayerContainer tdh)
      {
         cleanupLayerData(ref tdh);

         mChunkTextureList.Add(new XTDTextureLinkerData());
         XTDTextureLinkerData ctd = mChunkTextureList[mChunkTextureList.Count-1];
         ctd.gridLocX = gridLocX;
         ctd.gridLocY = gridLocY;

         int imgSize = (int)(BTerrainTexturing.getAlphaTextureWidth() * BTerrainTexturing.getAlphaTextureHeight());
         //we always have splat layers
         {

            ctd.layerData = new BTerrainLayerContainer();

            tdh.copyTo(ref ctd.layerData);
            ctd.layerData.fullLayerAlpha(0);

            ctd.layerData.sort();
         }
      } 
      void genTextureChunks()
      {
         clean();
         BTerrainQuadNode[] mLeafNodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();

         for (int i = 0; i < mLeafNodes.Length; i++)
         {
            int locX = (int)(mLeafNodes[i].getDesc().mMinXVert / BTerrainQuadNode.cMaxWidth);
            int locZ = (int)(mLeafNodes[i].getDesc().mMinZVert / BTerrainQuadNode.cMaxHeight);
            addTextureChunk(locX, locZ, mLeafNodes[i].mLayerContainer);
            mChunkTextureList[mChunkTextureList.Count - 1].mOwnerNode = mLeafNodes[i];


            //{
            //   if (!Directory.Exists("OutLayers"))
            //      Directory.CreateDirectory("OutLayers");
            //   for (int k = 0; k < mLeafNodes[i].mLayerContainer.getNumLayers(); k++)
            //   {
            //      BTerrainTexturingLayer l = mLeafNodes[i].mLayerContainer.giveLayer(k);
            //      if (l == null)
            //         continue;

            //      FileStream s = File.Open("OutLayers\\alphaLayer" + i + "_" + k + ".raw", FileMode.OpenOrCreate, FileAccess.Write);
            //      BinaryWriter f = new BinaryWriter(s);
                  
            //      f.Write(l.mAlphaLayer);
            //      f.Close();
            //      s.Close();
            //   }
            //}
            
          
         }
      }
      //-----------------------------
      //-----------------------------
      //-----------------------------
      public void checkMaterialLinks(string filename, ref bool sp, ref bool rm, ref bool em)
      {
         String fname = filename;

         String ext = ".ddx";// Path.GetExtension(fname);


         string sname = fname.Substring(0, fname.LastIndexOf("_df")) + "_em" + ext;
         em |= File.Exists(sname);
         sname = fname.Substring(0, fname.LastIndexOf("_df")) + "_rm" + ext;
         rm |= File.Exists(sname);
         sname = fname.Substring(0, fname.LastIndexOf("_df")) + "_sp" + ext;
         sp |= File.Exists(sname);
      }
      public void areSpecialPassNeeded(XTDTextureLinkerData linkerDat,
                                              ref int specularNeeded, ref int emissiveNeeded, ref int envMapNeeded, ref int alphaNeeded, ref int isFullyOpaque)
      {
         //walk our textures, see if there's an '_em' version of the texture
         bool specular = false;
         bool emissive = false;
         bool envMap = false;

         for (int i = 0; i < linkerDat.layerData.getNumLayers(); i++)
         {
            int texID = linkerDat.layerData.giveLayer(i).mActiveTextureIndex;
            if (linkerDat.layerData.giveLayer(i).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
            {
               checkMaterialLinks(TerrainGlobals.getTexturing().getActiveTexture(texID).mFilename, ref specular, ref envMap, ref emissive);
            }
            else if (linkerDat.layerData.giveLayer(i).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
            {
               int decalID = TerrainGlobals.getTexturing().getActiveDecalInstance(texID).mActiveDecalIndex;
               checkMaterialLinks(TerrainGlobals.getTexturing().getActiveDecal(decalID).mFilename, ref specular, ref envMap, ref emissive);
            }
         }



         specularNeeded = specular ? 1 : 0;
         emissiveNeeded = emissive ? 1 : 0;
         envMapNeeded = envMap ? 1 : 0;

         //check if we need alpha pass
         isFullyOpaque = 1;
         int numxChunks = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.getMaxNodeWidth());
         BTerrainQuadNode node = linkerDat.mOwnerNode;
         int minx = node.getDesc().mMinXVert;
         int minz = node.getDesc().mMinZVert;
         for (int i = 0; i < BTerrainQuadNode.getMaxNodeWidth(); i++)
         {
            for (int j = 0; j < BTerrainQuadNode.getMaxNodeDepth(); j++)
            {
               int index = (i + minx) * TerrainGlobals.getTerrain().getNumXVerts() + (j + minz);

               if (TerrainGlobals.getEditor().getAlphaValues()[index] != Byte.MaxValue) alphaNeeded = 1;
               if (TerrainGlobals.getEditor().getAlphaValues()[index] != Byte.MinValue) isFullyOpaque = 0;
            }
         }

      }
      //-----------------------------

      public unsafe void writeTextureChunks(ref ExportResults results)
      {
         DateTime n = DateTime.Now;
         genTextureChunks();

         for (int i = 0; i < mChunkTextureList.Count; i++)
            writeLinkerChunk(mChunkTextureList[i], ref results);

         TimeSpan ts = DateTime.Now - n;
         results.terrainTextureTime = ts.TotalMinutes;
      }

      private unsafe void writeLinkerChunk(XTDTextureLinkerData linkerDat, ref ExportResults results)
      {
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);


         binWriter.Write(Xbox_EndianSwap.endSwapI32(linkerDat.gridLocX));
         binWriter.Write(Xbox_EndianSwap.endSwapI32(linkerDat.gridLocY));

         //determine if special passes are needed.
         int emissiveNeeded = 0;
         int envMapNeeded = 0;
         int specualrNeeded = 0;
         int alphaNeeded = 0;
         int isFullyOpaque = 0;
         areSpecialPassNeeded(linkerDat, ref specualrNeeded, ref  emissiveNeeded, ref  envMapNeeded, ref alphaNeeded, ref isFullyOpaque);
         binWriter.Write(Xbox_EndianSwap.endSwapI32(specualrNeeded));
         binWriter.Write(Xbox_EndianSwap.endSwapI32(emissiveNeeded));
         binWriter.Write(Xbox_EndianSwap.endSwapI32(envMapNeeded));
         binWriter.Write(Xbox_EndianSwap.endSwapI32(alphaNeeded));
         binWriter.Write(Xbox_EndianSwap.endSwapI32(isFullyOpaque));


         //header info
         int numSplatLayers = linkerDat.layerData.getNumSplatLayers();
         int numAlignedSplatLayers = 4 * (((numSplatLayers - 1) >> 2) + 1);
         int numBlankSplatLayers = numAlignedSplatLayers - numSplatLayers;
         if (numSplatLayers > 1)
         {
            binWriter.Write(Xbox_EndianSwap.endSwapI32(numAlignedSplatLayers));
         }
         else
         {
            binWriter.Write(Xbox_EndianSwap.endSwapI32(1));
            numBlankSplatLayers = 0;
         }

         int numDecalLayers = linkerDat.layerData.getNumDecalLayers();
         int numAlignedDecalLayers = 4 * (((numDecalLayers - 1) >> 2) + 1);
         int numBlankDecalLayers = numAlignedDecalLayers - numDecalLayers;
         binWriter.Write(Xbox_EndianSwap.endSwapI32(numAlignedDecalLayers));




         int zero = 0;

         //write our splat data
         {
            //write our target active texture layer IDs
            for (int i = 0; i < linkerDat.layerData.getNumLayers(); i++)
            {
               if (linkerDat.layerData.giveLayer(i).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
                  binWriter.Write(Xbox_EndianSwap.endSwapI32(linkerDat.layerData.giveLayer(i).mActiveTextureIndex));
            }
            for (int i = 0; i < numBlankSplatLayers; i++)
               binWriter.Write(zero);


            //write our actual alpha layers
            if (numSplatLayers > 1)
               writePacked4bitLayers(binWriter, linkerDat, false, ref results);


         }

         if (numDecalLayers > 0)
         {
            //write our target active texture layer IDs
            for (int i = 0; i < linkerDat.layerData.getNumLayers(); i++)
            {
               if (linkerDat.layerData.giveLayer(i).mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
                  binWriter.Write(Xbox_EndianSwap.endSwapI32(linkerDat.layerData.giveLayer(i).mActiveTextureIndex));
            }
            for (int i = 0; i < numBlankDecalLayers; i++)
               binWriter.Write(zero);


            //write our actual alpha layers
            writePacked4bitLayers(binWriter, linkerDat, true, ref results);

         }

         ExportTo360.mECF.addChunk((int)eXTT_ChunkID.cXTT_TerrainAtlasLinkChunk, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;
      }
      //----------
      private XTDTextureLinkerData getNeighborLinker(int x, int z)
      {
         for (int i = 0; i < mChunkTextureList.Count; i++)
            if (mChunkTextureList[i].gridLocX == x && mChunkTextureList[i].gridLocY == z)
               return mChunkTextureList[i];

         return null;
      }

      public byte[] gutterAlphaTextureForBlending(BTerrainTexturingLayer layer, XTDTextureLinkerData rootNode)
      {
         int activeTexIndex = layer.mActiveTextureIndex;
         BTerrainTexturingLayer.eLayerType type = layer.mLayerType;

         int width = (int)BTerrainTexturing.getAlphaTextureWidth();
         int height = (int)BTerrainTexturing.getAlphaTextureHeight();
         int sliceSize = width * height;

         int lw = width + 2;
         int lh = height + 2;

         //Create our texture first, border it, then resize it.
         byte[] fullimg = new byte[lw * lh];
         //fill the origional texture into the new texture
         for (int q = 0; q < (width); q++)
         {
            for (int j = 0; j < (height); j++)
            {
               fullimg[(q + 1) + lw * (j + 1)] = layer.mAlphaLayer[(q + (width) * j)];
            }
         }


         XTDTextureLinkerData node = null;
         int layerIndex = 0;
         byte[] tmpAlpha = null;

         #region SIDES
         //grab neighbor alpha values, paste them into my edges.
         //LEFT SIDE FILL
         node = getNeighborLinker(rootNode.gridLocX - 1, rootNode.gridLocY);
         if ((node != null) &&
            ((layerIndex = node.layerData.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            {
               tmpAlpha = node.layerData.giveLayer(layerIndex).mAlphaLayer;

               //grab the RIGHT pixels
               for (int i = 0; i < height; i++)
               {
                  int srcIndex = (width - 1) + width * i;
                  int destIndex = 0 + (lw * (i + 1));
                  fullimg[destIndex] = tmpAlpha[srcIndex];
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
               fullimg[destIndex] = layer.mAlphaLayer[srcIndex];
            }
            
         }


         //RIGHT SIDE FILL
         node = getNeighborLinker(rootNode.gridLocX + 1, rootNode.gridLocY);
         if ((node != null) &&
            ((layerIndex = node.layerData.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            {
               tmpAlpha = node.layerData.giveLayer(layerIndex).mAlphaLayer;//type == BTerrainTexturingLayer.eLayerType.cLayer_Splat ? node.splatData.layersAlphas : node.decalData.layersAlphas;               
               //grab the LEFT pixels
               for (int i = 0; i < height; i++)
               {
                  int srcIndex = 0 + width * i;
                  int destIndex = (lw - 1) + (lw * (i + 1));
                  fullimg[destIndex] = tmpAlpha[srcIndex];
               }
            }
            
         }
         else
         {
            //extend the RIGHT pixels
               for (int i = 0; i < height; i++)
               {
                  int srcIndex = (width - 1) + width * i;
                  int destIndex = (lw - 1) + (lw * (i + 1));
                  fullimg[destIndex] = layer.mAlphaLayer[srcIndex];
               }
            
         }

         //TOP SIDE FILL
         node = getNeighborLinker(rootNode.gridLocX, rootNode.gridLocY + 1);
         if ((node != null) &&
            ((layerIndex = node.layerData.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            {
               tmpAlpha = node.layerData.giveLayer(layerIndex).mAlphaLayer;//type == BTerrainTexturingLayer.eLayerType.cLayer_Splat ? node.splatData.layersAlphas : node.decalData.layersAlphas;               
               //grab the BOTTOM pixels
               for (int i = 0; i < width; i++)
               {
                  int srcIndex = i + width * 0;
                  int destIndex = (i + 1) + (lw * (lh - 1));
                  fullimg[destIndex] = tmpAlpha[srcIndex];
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
                  fullimg[destIndex] = layer.mAlphaLayer[srcIndex];
               }
            
         }

         //BOTTOM SIDE FILL
         node = getNeighborLinker(rootNode.gridLocX, rootNode.gridLocY - 1);
         if ((node != null) &&
             ((layerIndex = node.layerData.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            layerIndex = node.layerData.giveLayerIndex(activeTexIndex, type);
            if (layerIndex > -1)
            {
               tmpAlpha = node.layerData.giveLayer(layerIndex).mAlphaLayer;//type == BTerrainTexturingLayer.eLayerType.cLayer_Splat ? node.splatData.layersAlphas : node.decalData.layersAlphas;               
               //grab the TOP pixels
               for (int i = 0; i < width; i++)
               {
                  int srcIndex = i + width * (height - 1);
                  int destIndex = (i + 1) + (lw * 0);
                  fullimg[destIndex] = tmpAlpha[srcIndex];
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
                  fullimg[destIndex] = layer.mAlphaLayer[srcIndex];
               }
            
         }
         #endregion

         #region CORNERS
         //TOP LEFT CORNER
         node = getNeighborLinker(rootNode.gridLocX - 1, rootNode.gridLocY + 1);
         if ((node != null) &&
            ((layerIndex = node.layerData.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            {
               tmpAlpha = node.layerData.giveLayer(layerIndex).mAlphaLayer;//type == BTerrainTexturingLayer.eLayerType.cLayer_Splat ? node.splatData.layersAlphas : node.decalData.layersAlphas;               
               //grab the BOTTOM RIGHT pixel
               //for (int i = 0; i < width; i++)
               {
                  int srcIndex = (width - 1) + width * 0;
                  int destIndex = 0 + (lw * (lh - 1));
                  fullimg[destIndex] = tmpAlpha[srcIndex];
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
                  fullimg[destIndex] = layer.mAlphaLayer[srcIndex];
               }
            
         }
         //TOP RIGHT CORNER
         node = getNeighborLinker(rootNode.gridLocX + 1, rootNode.gridLocY + 1);
         if ((node != null) &&
            ((layerIndex = node.layerData.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            {
               tmpAlpha = node.layerData.giveLayer(layerIndex).mAlphaLayer;//type == BTerrainTexturingLayer.eLayerType.cLayer_Splat ? node.splatData.layersAlphas : node.decalData.layersAlphas;               
               //grab the BOTTOM LEFT pixel
               //for (int i = 0; i < width; i++)
               {
                  int srcIndex = 0;// +width * (height - 1);
                  int destIndex = (lw - 1) + (lw * (lh - 1));
                  fullimg[destIndex] = tmpAlpha[srcIndex];
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
                  fullimg[destIndex] = layer.mAlphaLayer[srcIndex];
               }
            
         }
         //BOTTOM LEFT CORNER
         node = getNeighborLinker(rootNode.gridLocX - 1, rootNode.gridLocY - 1);
        if ((node != null) &&
            ((layerIndex = node.layerData.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            {
               tmpAlpha = node.layerData.giveLayer(layerIndex).mAlphaLayer;//type == BTerrainTexturingLayer.eLayerType.cLayer_Splat ? node.splatData.layersAlphas : node.decalData.layersAlphas;               
               //grab the TOP RIGHT pixel
               //for (int i = 0; i < width; i++)
               {
                  int srcIndex = (width - 1) + width * (height - 1);
                  int destIndex = 0 + (lw * 0);
                  fullimg[destIndex] = tmpAlpha[srcIndex];
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
                  fullimg[destIndex] = layer.mAlphaLayer[srcIndex];
               }
            
         }
         //BOTTOM RIGHT CORNER
         node = getNeighborLinker(rootNode.gridLocX + 1, rootNode.gridLocY - 1);
         if ((node != null) &&
            ((layerIndex = node.layerData.giveLayerIndex(activeTexIndex, type)) != -1))
         {
            {
               tmpAlpha = node.layerData.giveLayer(layerIndex).mAlphaLayer;//type == BTerrainTexturingLayer.eLayerType.cLayer_Splat ? node.splatData.layersAlphas : node.decalData.layersAlphas;               
               //grab the TOP LEFT pixel
               //for (int i = 0; i < width; i++)
               {
                  int srcIndex = (0) + width * (height - 1);
                  int destIndex = (lw - 1) + (lw * 0);
                  fullimg[destIndex] = tmpAlpha[srcIndex];
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
                  fullimg[destIndex] = layer.mAlphaLayer[srcIndex];
               }
            
         }

         #endregion


         //return ImageManipulation.resizeGreyScaleImg(fullimg, width + 2, height + 2, width, height, ImageManipulation.eFilterType.cFilter_Nearest);
         return ImageManipulation.ResampleGreyScaleImg(fullimg, width + 2, height + 2, width, height, ImageManipulation.eResamplerMethod.cRS_Box);
      }
      //----------
      private void intArrayToByteArrayToFile(BinaryWriter binWriter, Int16[] array)
      {
         byte[] bytearray = new byte[array.Length * sizeof(Int16)];
         int counter = 0;
         for (int i = 0; i < array.Length; i++)
         {
            bytearray[counter] = (byte)((array[i] & 0x0F00) >> 8);
            bytearray[counter++] |= (byte)((array[i] & 0xF000) >> 8);

            bytearray[counter] = (byte)((array[i] & 0x000F) >> 0);
            bytearray[counter++] |= (byte)((array[i] & 0x00F0) >> 0);
         }

         byte[] tempbytearray = new byte[array.Length * sizeof(Int16)];
         bytearray.CopyTo(tempbytearray, 0);
         ExportTo360.tileCopy(ref tempbytearray, tempbytearray, (int)BTerrainTexturing.getAlphaTextureWidth(), (int)BTerrainTexturing.getAlphaTextureHeight(), (int)ExportTo360.eTileCopyFormats.cTCFMT_R8G8);
         tempbytearray.CopyTo(bytearray, 0);

         binWriter.Write(bytearray);
         bytearray = null;
         tempbytearray = null;
      }
      private unsafe void writePacked4bitLayers(BinaryWriter binWriter, XTDTextureLinkerData linkerDat, bool whom, ref ExportResults results)
      {
         int arrLen = whom ? linkerDat.layerData.getNumDecalLayers() : linkerDat.layerData.getNumSplatLayers();
         //group layers into 4bpp
         int numAlignedGroups = ((arrLen - 1) >> 2) + 1;

         int numBlankLayers = (numAlignedGroups * 4) - arrLen;
         int width = (int)BTerrainTexturing.getAlphaTextureWidth();
         int height = (int)BTerrainTexturing.getAlphaTextureHeight();

         results.terrainTextureMemorySize += numAlignedGroups * sizeof(Int16) * width * height;



         Int16[] tmpLayerArray = new Int16[width * height];
         for (int i = 0; i < width * height; i++)
            tmpLayerArray[i] = 0;

         int[] shiftOffset = new int[4] { 8, 4, 0, 12 };
         int cCount = 0;
         int start = whom ? linkerDat.layerData.getNumLayers() - arrLen : 0;
         int end = whom ? linkerDat.layerData.getNumLayers() : arrLen;
         for (int i = start; i < end; i++)
         {
            int sliceIndex = (i * width * height);
            //gutter this texture for filtering here
            byte[] tmpAlphas = gutterAlphaTextureForBlending(linkerDat.layerData.giveLayer(i),
                                                             linkerDat);


            //CLM copy this to the temp holder, so our atlas construction can skip it..
            tmpAlphas.CopyTo(linkerDat.layerData.giveLayer(i).mAlphaLayer, 0);

            int index = 0;
            for (int x = 0; x < width; x++)
            {
               for (int y = 0; y < height; y++)
               {
                  Byte value = tmpAlphas[index];//whom ? linkerDat.decalData.layersAlphas[sliceIndex + index]
                  //:linkerDat.splatData.layersAlphas[sliceIndex + index];

                  tmpLayerArray[index] |= (Int16)(((int)((value / 255.0f) * 0xF)) << shiftOffset[cCount]);
                  index++;
               }
            }

            tmpAlphas = null;

            if (cCount == 3)
            {
               cCount = 0;
               intArrayToByteArrayToFile(binWriter, tmpLayerArray);
               for (int m = 0; m < width * height; m++)
                  tmpLayerArray[m] = 0;
            }
            else
               cCount++;

         }

         if (cCount % 4 != 0)
         {
            intArrayToByteArrayToFile(binWriter, tmpLayerArray);
         }

         tmpLayerArray = null;
         shiftOffset = null;
      }
      #endregion
   }
}