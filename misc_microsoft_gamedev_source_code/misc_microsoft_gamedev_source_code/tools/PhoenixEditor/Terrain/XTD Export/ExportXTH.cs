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


using SimEditor;     //These two are so we can include objects..
/*
 * CLM 03.29.07
 * This file is responsible for generating a height field of data needed by the 360 build for decals and sim data
 * 
 */

namespace Export360
{
   //--------------------------------------------
   public class XTHExporter
   {
      public void destroy()
      {

      }
      //-----------------------------------------------------------------------------------
      void binWriteMatrix(BinaryWriter binWriter, Matrix mat)
      {
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M11));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M12));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M13));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M14));

         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M21));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M22));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M23));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M24));

         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M31));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M32));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M33));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M34));

         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M41));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M42));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M43));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(mat.M44));
      }
      //-----------------------------------------------------------------------------------
      void headerToChunk()
      {
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         int version = (int)eFileVersions.cXTHVersion;
         
         binWriter.Write(Xbox_EndianSwap.endSwapI32(version));
         ExportTo360.mECF.addChunk((int)eXTH_ChunkID.cXTH_XTHHeader, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;
      }
      //-----------------------------------------------------------------------------------
      void heightFieldToChunk(HeightsGen hg,ref ExportResults results)
      {
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         binWriter.Write(Xbox_EndianSwap.endSwapI32((int)hg.mHeightFieldAttributes.mWidth));
         binWriter.Write(Xbox_EndianSwap.endSwapI32((int)hg.mHeightFieldAttributes.mHeight));

         binWriter.Write(Xbox_EndianSwap.endSwapF32(hg.mHeightFieldAttributes.mWorldMinY));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(hg.mHeightFieldAttributes.mWorldMaxY));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(hg.mHeightFieldAttributes.mWorldRangeY));

         binWriter.Write(Xbox_EndianSwap.endSwapF32(hg.mHeightFieldAttributes.mBounds.min.X));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(hg.mHeightFieldAttributes.mBounds.min.Y));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(hg.mHeightFieldAttributes.mBounds.min.Z));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(hg.mHeightFieldAttributes.mBounds.max.X));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(hg.mHeightFieldAttributes.mBounds.max.Y));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(hg.mHeightFieldAttributes.mBounds.max.Z));

         binWriteMatrix(binWriter, hg.mHeightFieldAttributes.mNormZToWorld);
         binWriteMatrix(binWriter, hg.mHeightFieldAttributes.mWorldToNormZ);


         binWriter.Write(Xbox_EndianSwap.endSwapI32(hg.mHeightFieldAttributes.mpTexelsHI.Length));
         int memSize = hg.mHeightFieldAttributes.mpTexelsHI.Length * 2 * sizeof(short);
         binWriter.Write(Xbox_EndianSwap.endSwapI32(memSize));

         //expect the texture as G16R16, NON FLOAT! Just 16 bits of short.
         int numShorts = hg.mHeightFieldAttributes.mpTexelsHI.Length * 2;
         short[] sArray = new short[numShorts];
         for (int i = 0; i < hg.mHeightFieldAttributes.mpTexelsHI.Length; i++)
         {
            sArray[i * 2 + 0] = (short)Xbox_EndianSwap.endSwapI16((ushort)hg.mHeightFieldAttributes.mpTexelsHI[i]);
            sArray[i * 2 + 1] = (short)Xbox_EndianSwap.endSwapI16((ushort)hg.mHeightFieldAttributes.mpTexelsLO[i]);
         }

         //tileswap
         short[] tempbytearray = new short[sArray.Length];
         sArray.CopyTo(tempbytearray, 0);
         ExportTo360.tileCopy(ref tempbytearray, tempbytearray, (int)hg.mHeightFieldAttributes.mWidth, (int)hg.mHeightFieldAttributes.mHeight, ExportTo360.eTileCopyFormats.cTCFMT_G16R16);
       //  tempbytearray.CopyTo(sArray, 0);
         for (int i = 0; i < tempbytearray.Length;i++ )
            binWriter.Write(tempbytearray[i]);

         ExportTo360.mECF.addChunk((int)eXTH_ChunkID.cXTH_TerrainHeightfield, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         results.terrainGPUHeighfieldMemory = memSize;
      }
      //-----------------------------------------------------------------------------------
      class DXT3_1111
      {
         int mWidth = 256;
         int mHeight = 256;
         int numBlocks = 0;
         Int64[] mDXTBlocks = null;

         public DXT3_1111(int width, int height)
         {
            mWidth = width;
            mHeight = height;
            int xBlocks = width >> 4;
            int yBlocks = height >> 2;
            numBlocks = xBlocks * yBlocks;
            mDXTBlocks = new Int64[numBlocks];
         }
         ~DXT3_1111()
         {
            mDXTBlocks = null;
         }

         static int cNumBits	   = 64;
         static int cSecAnd      = cNumBits - 1;
         static ulong cTopMask	   = 0x8000000000000000;

         //--------------------------------
         void setContainerState(ref Int64 value, bool state, Int64 mask)
         {
            int istate = state ? 1 : 0;
            (value) ^= (-istate ^ (value)) & mask;
         }

         //--------------------------------
         void setState(ref Int64 block, int index, bool state)
         {
            int sIndex = (index & cSecAnd);
            setContainerState(ref block,state, (Int64)(cTopMask>>sIndex));
         }
         //--------------------------------
         void setValueToImage(ref Int64[] pDat, int x, int y, bool value)
         {
            if (x < 0 || x >= mWidth || y < 0 || y >= mHeight || pDat == null)
               return;

            int numXBlocks = mWidth>>4;

            //determine our block
            int xBlockIndex = x >> 4;  // div 16
            int yBlockIndex = y >> 2;  // div 4

            Int64 block = pDat[(xBlockIndex + numXBlocks * yBlockIndex)];
            //determine the position we're joined in
            int xPixelIndex = 15-(x %16);   //CLM I had to flip this value to get them to appear right with DXT3A_1111
            int yPixelIndex = (y % 4);   
            int localIndex = (xPixelIndex + 16 * yPixelIndex);
            setState(ref block, localIndex, value);

            pDat[xBlockIndex + numXBlocks * yBlockIndex] = block;
         }

         public void setValue(int x, int y, bool val)
         {
            setValueToImage(ref mDXTBlocks,x,y,val);
         }
         public Int64 getBlock(int index)
         {
            return mDXTBlocks[index];
         }
         public int getNumBlocks()
         {
            return numBlocks;
         }
      };

      void heightFieldAlphaToChunk(ref ExportResults results)
      {
         //i think for visual reasons, the resolution of this alpha data needs to match
         //the density of the terrain alpha..

         BTerrainSimRep simRep = TerrainGlobals.getEditor().getSimRep();

         int width = TerrainGlobals.getTerrain().getNumXVerts();
         int height = TerrainGlobals.getTerrain().getNumXVerts();
         if (width < 256 || height < 256)
            return;  //don't write for small maps..

         byte[] AlphaVals = TerrainGlobals.getEditor().getAlphaValues();

         DXT3_1111 img = new DXT3_1111(width, height);


         //walk through all the terrain alpha values,
         for (int y = 0; y < height; y++) 
         {
            for (int x = 0; x < width; x++)
            {
               int index = x * width + y;
               bool visible = AlphaVals[index] > 0;// if it's invisible, so are we.

               //walk through the sim rep. If it's passable, we're visible.
               float normalizedX = x / (float)width;
               float normalizedZ = y / (float)height;
               bool obstructsLand = simRep.getDataTiles().isLandObstructedComposite(normalizedX, normalizedZ);
               visible |= !obstructsLand;

               
               img.setValue(x, y, visible);
              
            }
         }


         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         int packedWidth = width >> 2;
         binWriter.Write(Xbox_EndianSwap.endSwapI32(packedWidth));
         binWriter.Write(Xbox_EndianSwap.endSwapI32(height));

         int numBlocks = img.getNumBlocks();

         //numXBlocks-1 + numXBlocks * numYBlocks
         int texelMemSize = numBlocks * sizeof(Int64);// (((width >> 4) - 1) + (width >> 4) * ((height >> 2) - 1)) * sizeof(Int64);
         binWriter.Write(Xbox_EndianSwap.endSwapI32(texelMemSize));

         
         for (int i = 0; i < numBlocks; i++ )
         {
            binWriter.Write(img.getBlock(i));
         }

         ExportTo360.mECF.addChunk((int)eXTH_ChunkID.cXTH_TerrainHeightfieldAlpha, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         results.terrainGPUHeighfieldMemory += texelMemSize;

         img = null;
      }
      //-----------------------------------------------------------------------------------
      public bool export_XTH(string filename, ExportSettings expSettings, ref ExportResults results)
      {
         DateTime n = DateTime.Now;



         HeightsGen hg = new HeightsGen();
         hg.computeHeightField(256,256,true,BMathLib.unitZ, 8);

         headerToChunk();
         heightFieldToChunk(hg,ref results);   //turn our data into a ECF chunk
         heightFieldAlphaToChunk(ref results);
         bool exportOK = ExportTo360.safeECFFileWrite(filename, ".XTH");

         hg.destroy();


         TimeSpan ts = DateTime.Now - n;
         results.terrainGPUHeighfieldTime = ts.TotalMinutes;

         return exportOK;
      }



   }
}