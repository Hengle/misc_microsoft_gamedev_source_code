
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

using Terrain;
using Terrain.Refinement;
using SimEditor;
using EditorCore;
using Export360;


namespace Export360
{


    //--------------------------------------------
   public class XTDExporter
   {
     
      public void destroy()
      {
      }
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------


      //----------------------------------------------


      private unsafe int writeChunk(XTDTerrainChunk chunk, BinaryWriter mem)
      {
         mem.Write(Xbox_EndianSwap.endSwapI32(chunk.gridLocX));
         mem.Write(Xbox_EndianSwap.endSwapI32(chunk.gridLocZ));
         mem.Write(Xbox_EndianSwap.endSwapI32(chunk.maxVertStride));
         mem.Write(Xbox_EndianSwap.endSwapF32(chunk.mMin.X));
         mem.Write(Xbox_EndianSwap.endSwapF32(chunk.mMin.Y));
         mem.Write(Xbox_EndianSwap.endSwapF32(chunk.mMin.Z));
         mem.Write(Xbox_EndianSwap.endSwapF32(chunk.mMax.X));
         mem.Write(Xbox_EndianSwap.endSwapF32(chunk.mMax.Y));
         mem.Write(Xbox_EndianSwap.endSwapF32(chunk.mMax.Z));
         bool canCastShadows = true;
         if (chunk.mMax.Y - chunk.mMin.Y < 2)
            canCastShadows = false;
         mem.Write(canCastShadows);
     
         return 0;
         
      }
      private void generateGridChunks(ref XTDVisualHeader header)
      {
         //Create and write our flat terrain quadnode chunks

         BTerrainQuadNode[] mLeafNodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
         

         for (int i = 0; i < mLeafNodes.Length; i++)
         {
            int width = (int)BTerrainQuadNode.getMaxNodeWidth();
            ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
            chunkHolder.mDataMemStream = new MemoryStream();
            BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

            XTDTerrainChunk gridNode = new XTDTerrainChunk();

            int locX = mLeafNodes[i].getDesc().mMinXVert / width;
            int locZ = mLeafNodes[i].getDesc().mMinZVert / width;

            gridNode.gridLocX = locX;
            gridNode.gridLocZ = locZ;

            
            //calculate our chunk data

            gridNode.heightmapOnly = false;
            gridNode.maxVertStride = 64;

          
            //lets get our verts, normals, and ambOcclu
            
            int mnX = locX * width;
            int mnZ = locZ * width;

            BBoundingBox bb = new BBoundingBox();
            bb.empty();

            
            for (int z = 0; z < width + 1; z ++)
            {
               for (int x = 0; x < width + 1; x ++)
               {
                  int xVal = (int)(mnX + x);
                  int zVal = (int)(mnZ + z);
                  
                  Vector3 v = TerrainGlobals.getTerrain().getPos(xVal,zVal);

                  bb.addPoint(v);
               }
            }

            gridNode.mMin = bb.min;
            gridNode.mMax = bb.max;
            
            //IF WE CONTAIN FOLIAGE! increase the size of our bounding boxes..
            if (FoliageManager.isChunkUsed(mLeafNodes[i]))
            {
               BBoundingBox bbA = FoliageManager.giveMaxBBs();

               gridNode.mMin += bbA.min;
               gridNode.mMax += bbA.max;
            }

            

            //expand our box a tad
            float scle = TerrainGlobals.getTerrain().getTileScale();
            gridNode.mMin -= new Vector3(scle, scle, scle);
            gridNode.mMax += new Vector3(scle, scle, scle);

            //send the verts off to the compressor to be compressed properly into an image (using whatever technique)
            //be sure to assign a corolary ID so we can reference it later.
            writeChunk(gridNode, binWriter);

            //add this chunk to our main data stream
            ExportTo360.mECF.addChunk((int)eXTD_ChunkID.cXTD_TerrainChunk, chunkHolder, binWriter.BaseStream.Length);
            binWriter.Close();
            binWriter = null;
            chunkHolder.Close();
            chunkHolder = null;

         }

         mLeafNodes = null;

      }

      private byte giveMaxTesselation(ref ErrorMetricRefine refiner, int xOff, int zOff, int nodeWidth, float minortyBias)
      {
         int PatchWidth =17;
         int numP = 0;
         int numF = 0;
         refiner.giveStatsInArea(xOff, zOff, PatchWidth, PatchWidth, out numP, out numF);

         float rat = (float)numP / (float)numF;

         //CLM THIS MUST MATCH THE 360
            //float[] tessLevels ={ 15, 7, 3, 1 };
         byte maxTess = 3;
         if(numP ==0)
         {
            maxTess = 3;
         }
         else if(numF ==0)
         {
            maxTess = 0;
         }
         else
         {
            if (rat <= 0.25f)
               maxTess = 3;   //==1
            else if (rat <= 0.5f)
               maxTess = 2;   //==3
            else if (rat <= 0.75f)
               maxTess = 1;   //==7
            else //if (rat <= 1.0f)
               maxTess = 0;   //==15
         }

         return maxTess;
          
      }
      private void smoothTessNeighbors(byte[] tDat, int numXPatches, int numZPatches)
      {
         //for (int x = 0; x < numXPatches; x++)
         //{
         //   for (int z = 0; z < numZPatches; z++)
         //   {
         //      int xMinVert = x * nodeWidth;
         //      int zMinVert = z * nodeWidth;

         //      tDat[x + z * numXPatches] = giveMaxTesselation(ref refiner, xMinVert, zMinVert, nodeWidth, minorityBias);
         //   }
         //}
      }
      private void markOverrideTessellations(ErrorMetricRefine refiner)
      {
         JaggedContainer<byte> v = TerrainGlobals.getEditor().getJaggedTesselation();

         int width = TerrainGlobals.getTerrain().getNumXVerts();

         long id;
         byte maskValue;
         v.ResetIterator();
         while (v.MoveNext(out id, out maskValue))
         {
            if (maskValue == BTerrainEditor.cTesselationEmptyVal)
               continue;

            int x = (int)(id / width);
            int z = (int)(id - x * width);

            if (maskValue == (int)BTerrainEditor.eTessOverrideVal.cTess_Max)
               refiner.setMarkedPt(x, z, true);
            else if (maskValue == (int)BTerrainEditor.eTessOverrideVal.cTess_Min)
               refiner.setMarkedPt(x, z, false);
         }
      }
      private void writeTessData(bool refineTerrain, float eLOD, float minorityBias, ref ExportResults results)
      {
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);


         //write the 'header'
         int nodeWidth = 16;
         int numXPatches = (int)(TerrainGlobals.getTerrain().getNumXVerts() / (float)nodeWidth);
         int numZPatches = (int)(TerrainGlobals.getTerrain().getNumXVerts() / (float)nodeWidth);
         int numXChunks = (int)(TerrainGlobals.getTerrain().getNumXVerts() / (float)BTerrainQuadNode.getMaxNodeDepth());
         int numPatches = numXPatches * numZPatches;


         binWriter.Write(Xbox_EndianSwap.endSwapI32(numXPatches));
         binWriter.Write(Xbox_EndianSwap.endSwapI32(numZPatches));
         //do this for each patch


         byte[] tDat = new byte[numPatches];
         Vector4[] bbDat = new Vector4[numPatches * 2];

         //CLM - instead of writing floating point values (and taking up more memory)
            //write indexes into our discrete step array

         if (refineTerrain)
         {
            ErrorMetricRefine refiner = null;
            refiner = new ErrorMetricRefine();
            refiner.init(TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumZVerts());

            refiner.refine(eLOD);//, nodeWidth + 1);

            markOverrideTessellations(refiner);

           
            for (int x = 0; x < numXPatches; x++)
            {
               for (int z = 0; z < numZPatches; z++)
               {
                  int xMinVert = x * nodeWidth;
                  int zMinVert = z * nodeWidth;

                  tDat[x + z * numXPatches] = giveMaxTesselation(ref refiner, xMinVert, zMinVert, nodeWidth, minorityBias);
               }
            }
            refiner.destroy();
            refiner = null;

            smoothTessNeighbors(tDat, numXPatches, numZPatches);
         }
         else
         {
            for (int i = 0; i < numPatches; i++)
               tDat[i] = 0;

         }


         //write to the disk
         for (int i = 0; i < tDat.Length; i++)
            binWriter.Write(tDat[i]);


         //generate and write our bounding boxes
         for (int x = 0; x < numXPatches; x++)
         {
            for (int z = 0; z < numZPatches; z++)
            {

               Vector4 min = new Vector4(9000, 9000, 9000,0);
               Vector4 max = new Vector4(-9000, -9000, -9000,0);
               for (int i = 0; i < nodeWidth+1; i++)
               {
                  for (int j = 0; j < nodeWidth+1; j++)
                  {

                     int xVal = x * nodeWidth + i;
                     int zVal = z * nodeWidth + j;
                     if (xVal >= TerrainGlobals.getTerrain().getNumXVerts() || zVal >= TerrainGlobals.getTerrain().getNumXVerts())
                        continue;

                     long kIndex = xVal + TerrainGlobals.getTerrain().getNumXVerts() * zVal;
                     Vector3 v = TerrainGlobals.getTerrain().getPos(xVal,zVal);
                     if (v.X < min.X) min.X = v.X;
                     if (v.Y < min.Y) min.Y = v.Y;
                     if (v.Z < min.Z) min.Z = v.Z;
                     if (v.X > max.X) max.X = v.X;
                     if (v.Y > max.Y) max.Y = v.Y;
                     if (v.Z > max.Z) max.Z = v.Z;

                  }
               }

               //NOTE: THE GAME EXPECTS THIS DATA IN NON-SWIZZLED FORM

               int q = x + z * numZPatches ;
               q *= 2;

               bbDat[q] = min;
               bbDat[q + 1] = max;

            }
         }
         for (int i = 0; i < bbDat.Length; i++)
         {
            binWriter.Write(Xbox_EndianSwap.endSwapF32(bbDat[i].X));
            binWriter.Write(Xbox_EndianSwap.endSwapF32(bbDat[i].Y));
            binWriter.Write(Xbox_EndianSwap.endSwapF32(bbDat[i].Z));
            binWriter.Write(Xbox_EndianSwap.endSwapF32(bbDat[i].W));
         }


         ExportTo360.mECF.addChunk((int)eXTD_ChunkID.cXTD_TessChunk, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         results.terrainTessValuesMemorySize = tDat.Length + (bbDat.Length * sizeof(float) * 4);

         tDat = null;
         bbDat = null;
  
      }
      class PixelRGB24
      {
         public byte R;
         public byte G;
         public byte B;
      };
      private void writeLightData(ref ExportSettings settings, ref ExportResults results)
      {
         if (!LightManager.hasTerrainLightData())
            return;

         DateTime n = DateTime.Now;
         int width = TerrainGlobals.getTerrain().getNumXVerts();
         int height = TerrainGlobals.getTerrain().getNumZVerts();
         Vector3 [] lht = new Vector3[width * height];

         //CLM this needs to be changed for batch exporting!
         LightManager.rasterTerrainLightsToExportGrid(lht);

         //convert our data to a DXT1 texture!
         //CLM this is swapped all to shit... WTF!?
         
         byte[] inData = new byte[width * height * 4];
         int c = 0;
         //for (int i = 0; i < lht.Length; i++)
         
         for (int y = 0; y < height; y++)
         {
            for (int x = 0; x < width; x++)   
            {
               //as an encoding step, divide by our overbright (2)
               //now convert back to srgb
               int i = x * width + y;
               Vector3 v = lht[i];
               v.X = (float)Math.Sqrt(BMathLib.Clamp((float)(v.X * 0.5), 0, 1));
               v.Y = (float)Math.Sqrt(BMathLib.Clamp((float)(v.Y * 0.5), 0, 1));
               v.Z = (float)Math.Sqrt(BMathLib.Clamp((float)(v.Z * 0.5), 0, 1));

               inData[c++] = (byte)(BMathLib.Clamp(v.Z * 255 + 0.5f, 0, 255));
               inData[c++] = (byte)(BMathLib.Clamp(v.Y * 255 + 0.5f, 0, 255));
               inData[c++] = (byte)(BMathLib.Clamp(v.X * 255 + 0.5f, 0, 255));
               inData[c++] = 0;
            }
         }


         byte[] outDat = null;
         int outMemSize = 0;
         ExportTo360.toCompressedFormat(inData, width, height, sizeof(UInt32), BDXTFormat.cDXT1, settings.ExportTextureCompressionQuality, ref outDat, ref outMemSize);


         //endianswap this data...
         int count = outMemSize;
         for (int i = 0; i < count - 1; i += 2)
         {
            byte a = outDat[i];
            outDat[i] = outDat[i + 1];
            outDat[i + 1] = a;
         }

         //tileswap
         byte[] tempbytearray = new byte[outDat.Length ];
         outDat.CopyTo(tempbytearray, 0);
         ExportTo360.tileCopy(ref tempbytearray, tempbytearray, (int)width, (int)height, (int)ExportTo360.eTileCopyFormats.cTCFMT_DXT1);
         tempbytearray.CopyTo(outDat, 0);
         tempbytearray = null;

         //WRITE THE CHUNK
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         //just write the DXT1 texture
         binWriter.Write(Xbox_EndianSwap.endSwapI32(outMemSize));
         binWriter.Write(outDat);

         ExportTo360.mECF.addChunk((int)eXTD_ChunkID.cXTD_LightingChunk, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         results.terrainLightingMemorySize = outMemSize;
         TimeSpan ts = DateTime.Now - n;
         results.terrainLightingTime = ts.TotalMinutes;

         

         lht = null;
         inData = null;
         outDat = null;


      }

      #region AO
      public static byte[] packAOToDXT5A(float[] AoVals, int numXVerts)
      {

         byte[] inData = new byte[AoVals.Length * 4];
         int c = 0;
         for (int x = 0; x < numXVerts; x++)
         {
            for (int z = 0; z < numXVerts; z++)
            {
               int dstIndex = 4 * (x * numXVerts + z);
               int srcIndex = (x + numXVerts * z);

               float k = AoVals[srcIndex] > 1 ? 1 : AoVals[srcIndex];

               inData[dstIndex + 0] = 1;
               inData[dstIndex + 1] = 1;
               inData[dstIndex + 2] = 1;
               inData[dstIndex + 3] = (byte)(k * 255);
            }
         }
         //for (int i = 0; i < AoVals.Length; i++)
         //{
         //   float k = AoVals[i] > 1 ? 1 : AoVals[i];

         //   inData[c++] = 1;
         //   inData[c++] = 1;
         //   inData[c++] = 1;
         //   inData[c++] = (byte)(k * 255);
         //}

         byte[] outDat = null;
         int outMemSize = 0;
         ExportTo360.toCompressedFormat(inData, numXVerts, numXVerts, sizeof(UInt32), BDXTFormat.cDXT5A, eDXTQuality.cDXTQualityBest, ref outDat, ref outMemSize);


         //endianswap this data...
         int count = outMemSize;
         for (int i = 0; i < count - 1; i += 2)
         {
            byte a = outDat[i];
            outDat[i] = outDat[i + 1];
            outDat[i + 1] = a;
         }
         //swizzle this data

         inData = null;
         return outDat;
      }
      public void writeAOData(ref ExportResults results)
      {

         //WRITE THE CHUNK
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         byte[] data = packAOToDXT5A(TerrainGlobals.getEditor().getAmbientOcclusionValues(), TerrainGlobals.getTerrain().getNumXVerts());
         for (int i = 0; i < data.Length; i++)
            binWriter.Write(data[i]);
         results.terrainAOMemorySize = data.Length;
         data = null;

         ExportTo360.mECF.addChunk((int)eXTD_ChunkID.cXTD_AOChunk, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;
      }
      #endregion

      #region ALPHA
      byte[] packAlphaToDXT5A(byte[] AlphaVals, int numXVerts)
      {
         byte[] inData = new byte[AlphaVals.Length * 4];
         for (int x = 0; x < numXVerts; x++)
         {
            for (int z = 0; z < numXVerts; z++)
            {
               int dstIndex = 4 * (x * numXVerts + z);
               int srcIndex = (x + numXVerts * z);

               inData[dstIndex + 0] = 1;
               inData[dstIndex + 1] = 1;
               inData[dstIndex + 2] = 1;
               inData[dstIndex + 3] = AlphaVals[srcIndex];
            }
         }
         //int c = 0;
         //for (int i = 0; i < AlphaVals.Length; i++)
         //{
         //   inData[c++] = 1;
         //   inData[c++] = 1;
         //   inData[c++] = 1;
         //   inData[c++] = AlphaVals[i];
         //}

         byte[] outDat = null;
         int outMemSize = 0;
         ExportTo360.toCompressedFormat(inData, numXVerts, numXVerts, sizeof(UInt32), BDXTFormat.cDXT5A, eDXTQuality.cDXTQualityBest, ref outDat, ref outMemSize);


         //endianswap this data...
         int count = outMemSize;
         for (int i = 0; i < count - 1; i += 2)
         {
            byte a = outDat[i];
            outDat[i] = outDat[i + 1];
            outDat[i + 1] = a;
         }
         //swizzle this data

         return outDat;
      }

      void writeAlpha(ref ExportResults results)
      {
     

         //WRITE THE CHUNK
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);


         #region DXT5Alpha
         byte[] dataAlpha = packAlphaToDXT5A(TerrainGlobals.getEditor().getAlphaValues(), TerrainGlobals.getTerrain().getNumXVerts());
         for (int i = 0; i < dataAlpha.Length; i++)
            binWriter.Write(dataAlpha[i]);
         results.terrainAlphaMemorySize = dataAlpha.Length;
         dataAlpha = null;
         #endregion

         ExportTo360.mECF.addChunk((int)eXTD_ChunkID.cXTD_AlphaChunk, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;
      }
      #endregion

      #region Compressed Terrain
      struct chunkDataHolder
      {
         public int gridX;
         public int gridZ;
         public eECFChunkType mType;
         public int maxStride;
         public XTD3DVisual mVisualData;
      }

      int watchCount = 0;
      char maxBits = (char)127;
      float maxBitsF = 127.0f;


      enum eRangeReturn
      {
         cGood = 0,
         cRangeToLarge = 1,
         cRangeToSmall = 2
      };

      private int giveTexBlockSize(int count)
      {
         int c = 0;
         while ((int)Math.Pow(2, c) * (int)Math.Pow(2, c) < count)
            c++;

         int val = (int)Math.Pow(2, c);
         return val;
      }


      float rangeCompact(float r)
      {
         int mR = (int)(r + 1);
         if (mR % 2 == 0)
            return (float)mR;

         return (float)((int)r);
      }
      void findMaxsMinsRange( ref Vector3 max, ref Vector3 mid, ref Vector3 range)
      {
         BBoundingBox bb = TerrainGlobals.getTerrain().getDisplacementBB();
         max = bb.max;
         mid = bb.min;

         range = (max - mid) * 2.1f;

         mid = range * 0.5f;
         mid.X = rangeCompact(mid.X);
         mid.Y = rangeCompact(mid.Y);
         mid.Z = rangeCompact(mid.Z);

         range = (mid * 2.1f);

         if (range.X == 0) { mid.X = 0; range.X = 1; }
         if (range.Y == 0) { mid.Y = 0; range.Y = 1; }
         if (range.Z == 0) { mid.Z = 0; range.Z = 1; }
      }
      Vector3 calcAdjustedMidValue(Vector3 inputMid)
      {
         BBoundingBox bb = TerrainGlobals.getTerrain().getDisplacementBB();
         Vector3 diff = bb.max - bb.min;
         int qDiff = (int)(diff.Y / 100)*2;

         Vector3 adjustedMid = new Vector3(0, qDiff / 10.0f, 0);
         return inputMid -adjustedMid;
      }
      UInt32 packPosTo32Bits(Vector3 pos, Vector3 mid, Vector3 vrange)
      {

         Int32 outval = 0;
         //X = 11bits
         //Y = 11bits
         //Z = 10bits
         const int bitMax10 = 1023; //0x3ff
         const int bitMax9 = 511;   //0x1ff;


         //scale based upon range
         ushort range = (ushort)((((pos.X + mid.X) / vrange.X) * (bitMax10)));
         ushort output = (ushort)(range & bitMax10);
         outval |= output << 22;

         range = (ushort)(((pos.Y + mid.Y) / vrange.Y) * (bitMax10));
         output = (ushort)(range & bitMax10);
         outval |= output << 11;

         range = (ushort)((((pos.Z + mid.Z) / vrange.Z) * (bitMax10)));
         output = (ushort)(range & bitMax10);
         outval |= output;

         return (UInt32)outval;
      }
      UInt32 packNormTo32Bits(Vector3 norm)
      {
         Int32 outval = 0;
         //X = 11bits
         //Y = 11bits
         //Z = 10bits
         const int bitMax10 = 1023; //0x3ff
         const int bitMax9 = 511;   //0x1ff;

         //scale based upon range
         ushort range = (ushort)((((norm.X + 1) * 0.5f) * (bitMax10)));
         ushort output = (ushort)(range & bitMax10);
         outval |= output << 22;

         range = (ushort)(((norm.Y + 1) * 0.5f) * (bitMax10));
         output = (ushort)(range & bitMax10);
         outval |= output << 11;

         range = (ushort)((((norm.Z + 1) * 0.5f) * (bitMax10)));
         output = (ushort)(range & bitMax10);
         outval |= output;

         return (UInt32)outval;
      }
      
      
      public void writeAtlasToMemory(ref ExportResults results)
      {
         int len = TerrainGlobals.getTerrain().getNumZVerts() * TerrainGlobals.getTerrain().getNumXVerts();

         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);


         #region 32bit pos
         Vector3 min = new Vector3(9000, 9000, 9000);
         Vector3 max = new Vector3(-9000, -9000, -9000);
         Vector3 range = new Vector3(0, 0, 0);
         findMaxsMinsRange(ref max, ref min, ref range);

         Vector3 adjustedMidValue =  calcAdjustedMidValue(min);

         //write our mins & ranges (shoudl use XTDAtlasVisual here..)
         binWriter.Write(Xbox_EndianSwap.endSwapF32(adjustedMidValue.X));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(adjustedMidValue.Y));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(adjustedMidValue.Z));
         binWriter.Write(0);  //padd to allow 360 to read XMVECTOR from disk
         binWriter.Write(Xbox_EndianSwap.endSwapF32(range.X));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(range.Y));
         binWriter.Write(Xbox_EndianSwap.endSwapF32(range.Z));
         binWriter.Write(0);  //padd to allow 360 to read XMVECTOR from disk

         //CLM i'm trying this out...
         long currLen = binWriter.BaseStream.Position;
         binWriter.BaseStream.SetLength(currLen + ((sizeof(int)*len)*2));

         //make sure we swizzle it...
         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();
         Int32[] tmpArray = new Int32[len];
          for (int i = 0; i < len; i++)
             tmpArray[i] = (int)packPosTo32Bits(detail[i], min, range);
          
        
         ExportTo360.tileCopy(ref tmpArray, tmpArray, TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumZVerts());

         for (int i = 0; i < len; i++)
            binWriter.Write(Xbox_EndianSwap.endSwapI32((int)tmpArray[i]));//(int)packPosTo32Bits(expData.mTerrainPoints[i],min,range)));
         
         #endregion

         #region 32bit normal
         Vector3[] normals = TerrainGlobals.getEditor().getNormals();
          for (int i = 0; i < len; i++)
               tmpArray[i] = (int)packNormTo32Bits(normals[i]);
          

         ExportTo360.tileCopy(ref tmpArray, tmpArray, TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumZVerts());

         for (int i = 0; i < len; i++)
            binWriter.Write(Xbox_EndianSwap.endSwapI32((int)tmpArray[i]));//(int)packPosTo32Bits(expData.mTerrainPoints[i],min,range)));
         
         #endregion

         tmpArray = null;


         

         ExportTo360.mECF.addChunk((int)eXTD_ChunkID.cXTD_AtlasChunk, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         
         
         results.terrainPositionMemorySize = len * sizeof(UInt32);
         results.terrainNormalsMemorySize = len * sizeof(UInt32);

        
         
         normals = null;
         detail = null;
      }

      #endregion

      public bool export_XTD(string filename, ExportSettings expSettings, ref ExportResults results)
      {
         DateTime start = DateTime.Now;

         //write the header for the entire XTD file
         XTDVisualHeader header = new XTDVisualHeader();
         header.version = (int)eFileVersions.cXTDVersion;
         header.numXVerts = TerrainGlobals.getTerrain().getNumXVerts();
         header.numXChunks = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
         header.tileScale = TerrainGlobals.getTerrain().getTileScale();

         BBoundingBox simRepBB = SimGlobals.getSimMain().getBBoxForDecalObjects();
         Vector3 visRepMin = TerrainGlobals.getTerrain().getQuadNodeRoot().getDesc().m_min - new Vector3(0, header.tileScale, 0);
         Vector3 visRepMax = TerrainGlobals.getTerrain().getQuadNodeRoot().getDesc().m_max + new Vector3(0, header.tileScale, 0);

         if (!CoreGlobals.mbLoadTerrainVisRep)
         {
            visRepMin = simRepBB.min;
            visRepMax = simRepBB.max; 
         }
         

         header.worldMin = visRepMin;
         header.worldMax = visRepMax;
         header.endianSwap();


         //write our header first.
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);
         binWriter.Write(ExportTo360.StructToByteArray(header));
         ExportTo360.mECF.addChunk((int)eXTD_ChunkID.cXTD_XTDHeader, chunkHolder, (int)chunkHolder.mDataMemStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         header.endianSwap();


         generateGridChunks(ref header);

         writeAtlasToMemory(ref results);

         writeAOData(ref results);

         writeAlpha(ref results);

         writeTessData(expSettings.RefineTerrain, expSettings.RefineEpsilon, expSettings.RefineMinorityBias,ref results);

         writeLightData(ref expSettings,ref results);

         bool exportOK = ExportTo360.safeECFFileWrite(filename, ".XTD");

         
         TimeSpan ts = DateTime.Now - start;
         results.totalTime = ts.TotalMinutes;

         return exportOK;
      }


   }
}