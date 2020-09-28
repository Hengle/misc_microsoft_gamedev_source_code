using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Windows.Forms;

using EditorCore;
using SimEditor;
using Terrain;

//XBOX SIMREP DATA
namespace Export360
{

   //--------------------------------------------
   public class XSDExporter
   {
      const int cSimHeightResMultiplier = 1;

      public void destroy()
      {
      }
      public unsafe bool export_XSD(string filename, ExportSettings expSettings,ref Export360.ExportResults results)
      {
         DateTime n = DateTime.Now;

         //update before we do anything
         TerrainGlobals.getEditor().getSimRep().updateForExport();// update(false);//

         headerToChunk();

         results.terrainSimMemorySize = simDataToChunk();


         bool exportOK = ExportTo360.safeECFFileWrite(filename, ".XSD");

         TimeSpan ts = DateTime.Now - n;
         results.terrainSimTime = ts.TotalMinutes;

         return exportOK;
      }
      //-----------------------------------------
      private unsafe void headerToChunk()
      {
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         int version = (int)eFileVersions.cXSDVersion;
         binWriter.Write(Xbox_EndianSwap.endSwapI32(version));

         //sim terrain data stuff
         binWriter.Write(Xbox_EndianSwap.endSwapI32(TerrainGlobals.getEditor().getSimRep().getNumXTiles()));        //simTileScale
         binWriter.Write(Xbox_EndianSwap.endSwapF32(TerrainGlobals.getEditor().getSimRep().getTileScale()));        //simTileScale
         binWriter.Write(Xbox_EndianSwap.endSwapF32(TerrainGlobals.getEditor().getSimRep().getVisToSimScale()));    //visToSimScale

         
         //round our sim-rep up to next multiple of 8 for XBOX360 memory effecient tiling
         int width = (TerrainGlobals.getEditor().getSimRep().getNumXVerts() - 1) * cSimHeightResMultiplier + 1;
         int numXBlocks = width / 8;
         if (width % 8 != 0) numXBlocks++;
         int newWidth = numXBlocks * 8;

         float heightTileScale = (TerrainGlobals.getEditor().getSimRep().getTileScale() / (float)cSimHeightResMultiplier);


         binWriter.Write(Xbox_EndianSwap.endSwapI32(width));                    //numHeightVerts
         binWriter.Write(Xbox_EndianSwap.endSwapI32(newWidth ));                 //numHeightVertsCacheFeriently
         binWriter.Write(Xbox_EndianSwap.endSwapF32(heightTileScale ));                 //height tile scale
         binWriter.Write(Xbox_EndianSwap.endSwapI32(cSimHeightResMultiplier ));                 //DataToHeightMultiplier

         
         

      

         ExportTo360.mECF.addChunk((int)eXSD_ChunkID.cXSD_XSDHeader, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;
      }
      //-----------------------------------------
      float rangeCompact(float r)
      {
         int mR = (int)(r + 1);
         if (mR % 2 == 0)
            return (float)mR;

         return (float)((int)r);
      }
      void findMaxsMinsRange(ref float max, ref float mid, ref float range)
      {
         BBoundingBox bb = TerrainGlobals.getTerrain().getDisplacementBB();
         max = bb.max.Y;
         mid = bb.min.Y;

         range = (max - mid) * 2.1f;

         mid = range * 0.5f;
         mid = rangeCompact(mid);
         
         range = (mid * 2.1f);

         if (range == 0) { mid = 0; range = 1; }
      }
      UInt32 packPosTo32Bits(float pos, float mid, float vrange)
      {

         Int32 outval = 0;
         //X = 11bits
         //Y = 11bits
         //Z = 10bits
         const int bitMax10 = 1023; //0x3ff
         const int bitMax9 = 511;   //0x1ff;


         //scale based upon range
         ushort range = (ushort)(((pos + mid) / vrange) * (bitMax10));
         ushort output = (ushort)(range & bitMax10);
         outval |= output << 11;


         return (UInt32)outval;
      }
      void unpackVisualToPos(Int32 inpt, ref float val, ref float vMin, ref float vRange)
      {
         //X = 11bits
         //Y = 11bits
         //Z = 10bits
         const int bitMax10 = 1023; //0x3ff
         const float rcpBitMax10 = 1 / 1023.0f;
         //const int bitMax9 = 511;   //0x1ff;

         ushort sy = (ushort)((inpt >> 11) & bitMax10);
         
         val = (((sy * rcpBitMax10) * vRange) - vMin);
         
      }
      float calcAdjustedMidValue(float inputMid)
      {
         BBoundingBox bb = TerrainGlobals.getTerrain().getDisplacementBB();
         float diff = bb.max.Y - bb.min.Y;
         int qDiff = (int)(diff / 100) * 2;

         float adjustedMid =qDiff / 10.0f;
         return inputMid - adjustedMid;
      }


      private int simHeightsToChunk()
      {
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         int width = (TerrainGlobals.getEditor().getSimRep().getNumXVerts() - 1) * cSimHeightResMultiplier + 1;

  
         //float[] heights = TerrainGlobals.getEditor().getSimRep().getHeightRep().get.getSimHeights();
         //CLM : INFACT! Don't even consider doing this....
         //our res is good enough..
         //if (cSimHeightResMultiplier!=1)
         //{
         //   heights = new float[width * width];

         //   //CLM Generate a higher res Sim Heights rep 
         //   int xRes = 2048;
         //   float[] gpuHeightRepArray = new float[xRes * xRes];
         //   TerrainGlobals.getEditor().getSimRep().genHeightGPU(xRes, ref gpuHeightRepArray);

         //   float[] resPlusOne = ImageManipulation.resizeF32Img(gpuHeightRepArray, xRes, xRes, width - 1, width - 1, ImageManipulation.eFilterType.cFilter_Linear);//new float[xRes * xRes];
         //   for (int x = 0; x < width - 1; x++)
         //   {
         //      for (int z = 0; z < width - 1; z++)
         //      {
         //         heights[x * width + z] = resPlusOne[x * (width - 1) + z];
         //      }
         //   }

         //   //fill our edges with the previous height val.
         //   for (int q = 0; q < width - 1; q++)
         //   {
         //      heights[(width - 1) * width + q] = heights[(width - 2) * width + q];
         //      heights[q * width + (width - 1)] = heights[q * width + (width - 2)];
         //   }

         //   resPlusOne = null;
         //}

         //find our xbox specific terrain representation (close)
         float min = 9000;
         float max = -9000;
         float range =  0;
         findMaxsMinsRange(ref max, ref min, ref range);
         float adjustedMid = calcAdjustedMidValue(min);
         
         
         
         int numXBlocks = width / 8;
         if (width % 8 != 0) numXBlocks++;
         int newWidth = numXBlocks * 8;

         //swizzle our data to be more cache friendly..

         //first, padd our data to the next multiple of 8
         float[] tvals = new float[newWidth * newWidth];
         for (int i = 0; i < width; i++)
         {
            for (int k = 0; k < width; k++)
            {
               int srcIndex = i + k * width ;
               int dstIndex = i + k * newWidth;
               tvals[dstIndex] = TerrainGlobals.getEditor().getSimRep().getHeightRep().getCompositeHeight(k, i);//heights[srcIndex];

             //  int v = (int)packPosTo32Bits(tvals[dstIndex], min, range);
             //  unpackVisualToPos(v, ref tvals[dstIndex], ref adjustedMid, ref range);
               
            }
         }


         ////now swizzle the bastard.
         int blockSize = 8;
         float[] kvals = new float[newWidth * newWidth];
         int dstIndx = 0;
         for (int x = 0; x < numXBlocks; x++)
         {
            for (int z = 0; z < numXBlocks; z++)
            {
               int blockIndex = x + z * numXBlocks;
               //swizzle create this block
               for (int i = 0; i < blockSize; i++)
               {
                  for (int j = 0; j < blockSize; j++)
                  {
                     int k = (z * blockSize) + j;// (blockSize - 1 - j);
                     int l = (x * blockSize) + i;// (blockSize - 1 - i);

                     int srcIndx = k * newWidth + l;
                     //int dstIndx = i * blockSize + j;
                     kvals[dstIndx++] = tvals[srcIndx];
                  }
               }
            }
         }

         //FileStream st = File.Open("outTT.raw", FileMode.OpenOrCreate, FileAccess.Write);
         //BinaryWriter ft = new BinaryWriter(st);

         //convert to float16 & write to disk.
         float16[] sList = BMathLib.Float32To16Array(kvals);
         for (int c = 0; c < sList.Length; c++)
         {
            ushort s = ((ushort)sList[c].getInternalDat());
            binWriter.Write(Xbox_EndianSwap.endSwapI16(s));
            //ft.Write(s);
         }
         //ft.Close();
         //st.Close();


         ExportTo360.mECF.addChunk((int)eXSD_ChunkID.cXSD_SimHeights, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         //heights = null;

         return tvals.Length * sizeof(ushort);
      }
      private int writeObstructions()
      {
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);


         BTerrainSimRep simRep = TerrainGlobals.getEditor().getSimRep();

         // Write obstructions
         bool[] landObstructions = simRep.getDataTiles().getSimLandObstructions();
         int numTiles = simRep.getNumXTiles();
         int obstructionArraySize = numTiles * numTiles;
         byte[] obstructionsFlags = new byte[obstructionArraySize];

         bool[] alreadyDone = new bool[numTiles * numTiles];
         for (int i = 0; i < numTiles * numTiles; i++)
         {
            alreadyDone[i] = false;
         }

         for (int x = 0; x < numTiles; x++)
         {
            for (int z = 0; z < numTiles; z++)
            {
               int index = (x * numTiles + z);


               bool obstructsLand = simRep.getDataTiles().isLandObstructedComposite(x, z);
               bool obstructsFlood = simRep.getDataTiles().isFloodObstructed(x, z);
               bool obstructsScarab = simRep.getDataTiles().isScarabObstructed(x, z);


               obstructionsFlags[index] = 0;

               if       (obstructsLand && obstructsFlood && obstructsScarab)
                  obstructionsFlags[index] = (byte)BTerrainSimRep.eObstructionType.cObstrtuction_Land_Flood_Scarab;
               else if  (obstructsLand && obstructsFlood && !obstructsScarab)
                  obstructionsFlags[index] = (byte)BTerrainSimRep.eObstructionType.cObstrtuction_Land_Flood;
               else if  (obstructsLand && !obstructsFlood && obstructsScarab)  
                  obstructionsFlags[index] = (byte)BTerrainSimRep.eObstructionType.cObstrtuction_Land_Scarab;
               else if  (obstructsLand && !obstructsFlood && !obstructsScarab) 
                  obstructionsFlags[index] = (byte)BTerrainSimRep.eObstructionType.cObstrtuction_Land;
               else if (!obstructsLand && obstructsFlood && obstructsScarab)  
                  obstructionsFlags[index] = (byte)BTerrainSimRep.eObstructionType.cObstrtuction_Flood_Scarab;
               else if (!obstructsLand && obstructsFlood && !obstructsScarab) 
                  obstructionsFlags[index] = (byte)BTerrainSimRep.eObstructionType.cObstrtuction_Flood;
               else if (!obstructsLand && !obstructsFlood && obstructsScarab) 
                  obstructionsFlags[index] = (byte)BTerrainSimRep.eObstructionType.cObstrtuction_Scarab;
               else if (!obstructsLand && !obstructsFlood && !obstructsScarab) 
                  obstructionsFlags[index] = (byte)BTerrainSimRep.eObstructionType.cNoObstruction;
               
            }
         }

         binWriter.Write(obstructionsFlags);
         
         ExportTo360.mECF.addChunk((int)eXSD_ChunkID.cXSD_Obstructions, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         return obstructionsFlags.Length;
      }
      private int writeBuildable()
      {
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);


         BTerrainSimRep simRep = TerrainGlobals.getEditor().getSimRep();

         // Write buildable
         bool[] landBuildable = simRep.getDataTiles().getSimLandBuildable();
         int numTiles = simRep.getNumXTiles();

         byte tru = 0xFF;
         byte fal = 0x00;

         for (int x = 0; x < numTiles; x++)
         {
            for (int z = 0; z < numTiles; z++)
            {
               int index = (x * numTiles + z);

               if(landBuildable[index])
                  binWriter.Write(tru);
               else
                  binWriter.Write(fal);
            }
         }

         

         ExportTo360.mECF.addChunk((int)eXSD_ChunkID.cXSD_Buildable, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         return landBuildable.Length * sizeof(byte);
      }
      private int writeTileType()
      {
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         BTerrainSimRep simRep = TerrainGlobals.getEditor().getSimRep();


         // Write tile types
         int numTiles = simRep.getNumXTiles();
         int obstructionArraySize = numTiles * numTiles;
         SimTerrainType.loadTerrainTileTypes();
         int tileTypeArraySize = obstructionArraySize;
         
         byte[] tileTypeArray = new byte[tileTypeArraySize];
         for (int x = 0; x < numTiles; x++)
         {
            for (int z = 0; z < numTiles; z++)
            {
               int index = (x * numTiles + z);

               //if we have a tile type override, use it
               int typeOverride = TerrainGlobals.getEditor().getSimRep().getDataTiles().getJaggedTileType(z, x);
               if(typeOverride >0)
               {
                  tileTypeArray[index] = (byte)typeOverride;
               }
               else
               {
                  // Get each tile's dominant texture index
                  int dominantTextureIndex = TerrainGlobals.getEditor().giveDominantTextureAtTile(x, z, simRep);

                  tileTypeArray[index] = 0;
                  if (dominantTextureIndex != -1)
                  {
                     byte numTypes = (byte)SimTerrainType.mTerrainTileTypes.mTypes.Count;
                     string dominantTextureType = SimTerrainType.mTextureList[dominantTextureIndex].TileType;
                     // Find the dominant texture's type
                     for (byte typeIndex = 0; typeIndex < numTypes; typeIndex++)
                     {
                        if (SimTerrainType.mTerrainTileTypes.mTypes[typeIndex].Name == dominantTextureType)
                        {
                           tileTypeArray[index] = typeIndex;
                           break;
                        }
                     }
                  }
               }
               
            }
         }

         binWriter.Write(tileTypeArray);
         
         ExportTo360.mECF.addChunk((int)eXSD_ChunkID.cXSD_TileTypes, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         return tileTypeArray.Length;
      }
      private int writeCameraHeights()
      {
         TerrainGlobals.getEditor().getCameraRep().recalculateHeights();

         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);


         int width = (int)TerrainGlobals.getEditor().getCameraRep().getNumXPoints();
         int numXBlocks = width / 8;
         if (width % 8 != 0) numXBlocks++;
         int newWidth = numXBlocks * 8;

         //write our header information.
         float heightTileScale = TerrainGlobals.getEditor().getCameraRep().getTileScale();


         binWriter.Write(Xbox_EndianSwap.endSwapI32(width));                    //numHeightVerts
         binWriter.Write(Xbox_EndianSwap.endSwapI32(newWidth));                 //numHeightVertsCacheFeriently
         binWriter.Write(Xbox_EndianSwap.endSwapF32(heightTileScale));                 //height tile scale



         //swizzle our data to be more cache friendly..

         //first, padd our data to the next multiple of 8
         float[] tvals = new float[newWidth * newWidth];
         for (int i = 0; i < width; i++)
         {
            for (int k = 0; k < width; k++)
            {
               int dstIndex = i + k * newWidth;
               tvals[dstIndex] = TerrainGlobals.getEditor().getCameraRep().getCompositeHeight(k, i);// heights[srcIndex];
            }
         }


         ////now swizzle the bastard.
         int blockSize = 8;
         float[] kvals = new float[newWidth * newWidth];
         int dstIndx = 0;
         for (int x = 0; x < numXBlocks; x++)
         {
            for (int z = 0; z < numXBlocks; z++)
            {
               int blockIndex = x + z * numXBlocks;
               //swizzle create this block
               for (int i = 0; i < blockSize; i++)
               {
                  for (int j = 0; j < blockSize; j++)
                  {
                     int k = (z * blockSize) + j;// (blockSize - 1 - j);
                     int l = (x * blockSize) + i;// (blockSize - 1 - i);

                     int srcIndx = k * newWidth + l;
                     //int dstIndx = i * blockSize + j;
                     kvals[dstIndx++] = tvals[srcIndx];
                  }
               }
            }
         }

    
         //convert to float16 & write to disk.
         float16[] sList = BMathLib.Float32To16Array(kvals);
         for (int c = 0; c < sList.Length; c++)
         {
            ushort s = ((ushort)sList[c].getInternalDat());
            binWriter.Write(Xbox_EndianSwap.endSwapI16(s));
         }


         ExportTo360.mECF.addChunk((int)eXSD_ChunkID.cXSD_CameraHeights, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         return tvals.Length * sizeof(ushort);
         
      }
      private int writeFlightHeights()
      {
         TerrainGlobals.getEditor().getSimRep().getFlightRep().recalculateHeights();

         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);


         int width = (int)TerrainGlobals.getEditor().getSimRep().getFlightRep().getNumXPoints();
         int numXBlocks = width / 8;
         if (width % 8 != 0) numXBlocks++;
         int newWidth = numXBlocks * 8;

         //write our header information.
         float heightTileScale = TerrainGlobals.getEditor().getSimRep().getFlightRep().getTileScale();


         binWriter.Write(Xbox_EndianSwap.endSwapI32(width));                    //numHeightVerts
         binWriter.Write(Xbox_EndianSwap.endSwapI32(newWidth));                 //numHeightVertsCacheFeriently
         binWriter.Write(Xbox_EndianSwap.endSwapF32(heightTileScale));                 //height tile scale



         //swizzle our data to be more cache friendly..

         //first, padd our data to the next multiple of 8
         float[] tvals = new float[newWidth * newWidth];
         for (int i = 0; i < width; i++)
         {
            for (int k = 0; k < width; k++)
            {
               int dstIndex = i + k * newWidth;
               tvals[dstIndex] = TerrainGlobals.getEditor().getSimRep().getFlightRep().getCompositeHeight(k, i);// heights[srcIndex];
            }
         }


         ////now swizzle the bastard.
         int blockSize = 8;
         float[] kvals = new float[newWidth * newWidth];
         int dstIndx = 0;
         for (int x = 0; x < numXBlocks; x++)
         {
            for (int z = 0; z < numXBlocks; z++)
            {
               int blockIndex = x + z * numXBlocks;
               //swizzle create this block
               for (int i = 0; i < blockSize; i++)
               {
                  for (int j = 0; j < blockSize; j++)
                  {
                     int k = (z * blockSize) + j;// (blockSize - 1 - j);
                     int l = (x * blockSize) + i;// (blockSize - 1 - i);

                     int srcIndx = k * newWidth + l;
                     //int dstIndx = i * blockSize + j;
                     kvals[dstIndx++] = tvals[srcIndx];
                  }
               }
            }
         }


         //convert to float16 & write to disk.
         float16[] sList = BMathLib.Float32To16Array(kvals);
         for (int c = 0; c < sList.Length; c++)
         {
            ushort s = ((ushort)sList[c].getInternalDat());
            binWriter.Write(Xbox_EndianSwap.endSwapI16(s));
         }


         ExportTo360.mECF.addChunk((int)eXSD_ChunkID.cXSD_FlightHeights, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;

         return tvals.Length * sizeof(ushort);

      }
      private int simDataToChunk()
      {
         int memoryUsed = 0;
         memoryUsed += simHeightsToChunk();
         memoryUsed += writeObstructions();
         memoryUsed += writeBuildable();
         memoryUsed += writeTileType();
         memoryUsed += writeCameraHeights();
         memoryUsed += writeFlightHeights();

         return memoryUsed;
      }
      //-------------------------------------------
      //public void exportPTH(string filename, ref Export360.ExportResults results)
      //{
      //   DateTime n = DateTime.Now;
      //      filename = Path.ChangeExtension(filename, ".PTH");
      //      XSD_NavMesh nm = new XSD_NavMesh();
      //      nm.generateNavMesh(TerrainGlobals.getEditor().getSimRep().getNavMeshQuantizationDist(),512);
      //      nm.exportNavMeshToPTH(filename);
      //      nm = null;
      //      TimeSpan ts = DateTime.Now - n;
      //      results.terrainPTHTime = ts.TotalMinutes;

      //}
      //---------------------------------
      public bool CreateLRP(string filename)
      {
         try
         {
           

            //genPTH -terrainFile <*.XSD> -lrpTreeFile <*.LRP>
            if (File.Exists(CoreGlobals.getWorkPaths().mLRPToolPath) == false)
            {
               MessageBox.Show("Can't find: " + CoreGlobals.getWorkPaths().mLRPToolPath, "Error exporting " + filename);
               return false;
            }

            string xsdFile = filename.Substring(0, filename.LastIndexOf('.'));// Path.ChangeExtension(filename, "");
            string lrpFile = Path.ChangeExtension(filename, "LRP");

            if (File.Exists(lrpFile))
               File.Delete(lrpFile);

            string arguments = "";
            arguments = arguments + " -terrainFile \"" + xsdFile + "\"";
            arguments = arguments + " -lrpTreeFile \"" + lrpFile + "\"";

            System.Diagnostics.Process pthUtility;
            pthUtility = new System.Diagnostics.Process();
            pthUtility = System.Diagnostics.Process.Start(CoreGlobals.getWorkPaths().mLRPToolPath, arguments);
            pthUtility.WaitForExit();
            pthUtility.Close();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         return true;
      }
   }
}
