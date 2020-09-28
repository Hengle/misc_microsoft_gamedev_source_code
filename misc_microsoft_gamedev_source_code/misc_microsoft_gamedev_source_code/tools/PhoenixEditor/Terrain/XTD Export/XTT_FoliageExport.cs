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


/*
 * CLM 05.23.07
 * This class is responsible for exporting the foliage into 360 friendly sets
 */
namespace Export360
{
   public class XTT_FoliageExport
   {

      public void writeHeaderToChunk()
      {
         ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
         chunkHolder.mDataMemStream = new MemoryStream();
         BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

         List<FoliageSet> validSets = new List<FoliageSet>();
         for(int i=0;i<FoliageManager.getNumSets();i++)
         {
            if (FoliageManager.isSetUsed(FoliageManager.giveSet(i)))
               validSets.Add(FoliageManager.giveSet(i));
         }

         binWriter.Write(Xbox_EndianSwap.endSwapI32(validSets.Count));
         for(int i=0;i<validSets.Count;i++)
         {
            string tName = validSets[i].mFullFileName;
            if (tName.Contains(".xml"))
               tName = tName.Substring(0, tName.LastIndexOf(".xml"));

            string fName = tName.Remove(0, CoreGlobals.getWorkPaths().mGameArtDirectory.Length + 1);
            char[] filename = new char[256];
            fName.CopyTo(0, filename, 0, fName.Length);
            binWriter.Write(filename);

            ExportTo360.addTextureChannelDependencies(tName);
            if (File.Exists(Path.ChangeExtension(tName,".xml")))
               ExportTo360.mDEP.addFileDependent(Path.ChangeExtension(tName, ".xml"),false);
         }


         ExportTo360.mECF.addChunk((int)eXTT_ChunkID.cXTT_FoliageHeaderChunk, chunkHolder, binWriter.BaseStream.Length);
         binWriter.Close();
         binWriter = null;
         chunkHolder.Close();
         chunkHolder = null;
      }

      public void writeQNsToChunk(ref ExportResults results)
      {
         DateTime n = DateTime.Now;
         int totalMemory = 0;

         int width = TerrainGlobals.getTerrain().getNumXVerts();

         for (int qi = 0; qi < FoliageManager.getNumChunks(); qi++)
         {
            FoliageQNChunk chunk = FoliageManager.getChunk(qi);

            List<int> setIndexes = new List<int>();
            List<int> setPolyCounts = new List<int>();
            int totalPhysicalMemory = 0;
            List<int> indMemSizes = new List<int>();
            List<int> bladeIBs = new List<int>();

            for (int setI = 0; setI < chunk.mSetsUsed.Count; setI++)
            {
               int numBlades =0;
               int startIndex = bladeIBs.Count;
               FoliageSet fs = FoliageManager.giveSet(chunk.mSetsUsed[setI]);
               
               //walk through the main grid for our current chunk
               //pack the data into proper inds..
               for (int x = 0; x < BTerrainQuadNode.cMaxWidth; x++)
               {
                  for (int z = 0; z < BTerrainQuadNode.cMaxWidth; z++)
                  {
                     int index = (x + chunk.mOwnerNodeDesc.mMinXVert) + width * (z + chunk.mOwnerNodeDesc.mMinZVert);
                     FoliageVertData fvd = FoliageManager.mVertData.GetValue(index);
                     if (fvd.compare(FoliageManager.cEmptyVertData))
                        continue;

                     if (FoliageManager.giveIndexOfSet(fvd.mFoliageSetName) == FoliageManager.giveIndexOfSet(chunk.mSetsUsed[setI]))
                     {
                        numBlades++;

                        //our local blade index is transposed on the 360
                        int localIndex = x * FoliageManager.cNumXBladesPerChunk + z;
                        int bladeIndex = fvd.mFoliageSetBladeIndex<<16;

                        for (short t = 0; t < fs.mNumVertsPerBlade; t++)
                        {
                           int packedID = bladeIndex | (0xFFFF & (localIndex * fs.mNumVertsPerBlade) + t);
                           bladeIBs.Add(packedID);
                        }
                        bladeIBs.Add(0xFFFF);  //360 specific tag to stop a strip.
                     }
                  }
               }

               int numVerts = bladeIBs.Count - startIndex;
               if (numVerts == 0)
                  continue;

               setIndexes.Add(FoliageManager.giveIndexOfSet(chunk.mSetsUsed[setI]));
               setPolyCounts.Add(numBlades * (fs.mNumVertsPerBlade + 1) - 2);
               indMemSizes.Add(numVerts * sizeof(int));
               totalPhysicalMemory += indMemSizes[indMemSizes.Count - 1];

            }

            //now write this chunk..
            ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
            chunkHolder.mDataMemStream = new MemoryStream();
            BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

            //our chunk index is transposed in the game
            int minQNX = (int)(chunk.mOwnerNodeDesc.mMinXVert / BTerrainQuadNode.cMaxWidth);
            int minQNZ = (int)(chunk.mOwnerNodeDesc.mMinZVert / BTerrainQuadNode.cMaxHeight);
            int numXChunks = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
            int transposedOwnerIndex = minQNX * numXChunks + minQNZ;
            
            binWriter.Write(Xbox_EndianSwap.endSwapI32(transposedOwnerIndex));//chunk.mOwnerQNIndex));
            binWriter.Write(Xbox_EndianSwap.endSwapI32(setIndexes.Count));

            int numSets = setIndexes.Count;
            for (int i = 0; i < numSets;i++ )
               binWriter.Write(Xbox_EndianSwap.endSwapI32(setIndexes[i]));

            for (int i = 0; i < numSets; i++)
               binWriter.Write(Xbox_EndianSwap.endSwapI32(setPolyCounts[i]));


            binWriter.Write(Xbox_EndianSwap.endSwapI32(totalPhysicalMemory));
            totalMemory += totalPhysicalMemory;

            for (int i = 0; i < numSets; i++)
               binWriter.Write(Xbox_EndianSwap.endSwapI32(indMemSizes[i]));

            for (int i = 0; i < bladeIBs.Count; i++)
            {
               binWriter.Write(Xbox_EndianSwap.endSwapI32(bladeIBs[i]));
            }
            

            ExportTo360.mECF.addChunk((int)eXTT_ChunkID.cXTT_FoliageQNChunk, chunkHolder, binWriter.BaseStream.Length);
            binWriter.Close();
            binWriter = null;
            chunkHolder.Close();
            chunkHolder = null;
         }

         TimeSpan ts = DateTime.Now - n;
         results.terrainFoliageTime = ts.TotalMinutes;
         results.terrainFoliageMemory   = totalMemory;
      }

      public void exportFoliage(ref ExportResults results)
      {
         if (FoliageManager.getNumChunks() == 0)
            return;

         FoliageManager.removeUnusedSets();

         //write our headers..
         writeHeaderToChunk();

         writeQNsToChunk(ref results);
      }
   }
}