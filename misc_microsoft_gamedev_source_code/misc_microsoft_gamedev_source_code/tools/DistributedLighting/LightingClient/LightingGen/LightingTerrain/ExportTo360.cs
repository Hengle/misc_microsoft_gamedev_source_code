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
using ModelSystem;

/*
 * CLM 04.02.07
 * 
 * 
 */

//--------------------
namespace Export360
{
   //---------------------------------------
   //---------------------------------------
   class Xbox_EndianSwap
   {
      //-----------------------------------------------------------------------------
      public static ushort endSwapI16(ushort i)
      {
         return (ushort)((i << 8) | (i >> 8));
      }
      //-----------------------------------------------------------------------------
      public static int endSwapI32(int i)
      {
         return endSwapI16((ushort)(i & 0x0000FFFF)) << 16 | endSwapI16((ushort)(i >> 16));
      }
      //-----------------------------------------------------------------------------
      public static Int64 endSwapI64(Int64 i)
      {
         Int64 big = endSwapI32((Int32)(i & 0x00000000FFFFFFFF));
         big = big << 32;
         Int32 small = endSwapI32((Int32)(i >> 32));
         return big | small;
      }

      //-----------------------------------------------------------------------------
      public static float endSwapF32(float f)
      {
         byte[] b = BitConverter.GetBytes(f);
         Array.Reverse(b);

         return BitConverter.ToSingle(b, 0);
      }
   }


   //---------------------------------------
   //---------------------------------------
   public class ExportTo360
   {
     
      
      static private String mOutputOverrideDir = null;

      public static void checkLoadRender()
      {
         if (mExportTerrainRender==null)
         {
            mExportTerrainRender = new ExportRenderTerrain();
            mExportTerrainRender.init();
         }
      }
      public static void checkUnloadRender()
      {
         if (mExportTerrainRender != null)
         {
            
               mExportTerrainRender.destroy(); mExportTerrainRender = null;
            
         }
      }
      public static ExportRenderTerrain mExportTerrainRender = null;

      //dll functions in EditorUtils.dll
      #region 

      [DllImport("EditorUtils.dll", SetLastError = true)]
      static extern unsafe bool tileCopyData(void* dst, void* src, int Width, int Height, int dxtFormat, int pixelMemSize);
      [DllImport("EditorUtils.dll", SetLastError = true)]
      static extern unsafe void CompressToDXTN(Vector3[] CoeffData, int numXBlocks, int numZBlocks, float rangeH, float rangeV, byte** DXTN, ref int DXTNSize);
      [DllImport("EditorUtils.dll", SetLastError = true)]
      static extern unsafe void CompressToDXTNDirect(byte[] data, int numXBlocks, int numZBlocks, byte** DXTN, ref int dxtnSize);
      [DllImport("EditorUtils.dll", SetLastError = true)]
      static extern unsafe void CompressToFormat(byte[] data, int numXPixels, int numZPixels, int pixelMemSize, BDXTFormat desiredFormat,eDXTQuality quality, byte** outTexture, ref int outSize);
      [DllImport("EditorUtils.dll", SetLastError = true)]
      static extern unsafe void FreeCompressedData(byte** outTexture);

      static public unsafe void toCompressedFormat(byte[] data, int numXPixels, int numZPixels, int pixelMemSize, BDXTFormat desiredFormat, eDXTQuality quality, ref byte[] outImg, ref int memLen)
      {
         byte* imgDat = null;
         CompressToFormat(data, numXPixels, numZPixels, pixelMemSize, desiredFormat, quality, & imgDat, ref memLen);

         IntPtr p = new IntPtr(imgDat);

         outImg = new byte[memLen];

         System.Runtime.InteropServices.Marshal.Copy(p, outImg, 0, memLen);

         FreeCompressedData(&imgDat);
         imgDat = null;
         
      }
      
      static public unsafe void toDXTN(Vector3[] coeffs, int numXBlocks, int numZBlocks, float rangeH, float rangeV, ref byte[] DXTN, ref int memLen)
      {
         byte* DXNData = null;
         CompressToDXTN(coeffs, numXBlocks, numZBlocks, rangeH, rangeV, &DXNData, ref memLen);
         IntPtr p = new IntPtr(DXNData);

         DXTN = new byte[memLen];

         System.Runtime.InteropServices.Marshal.Copy(p, DXTN, 0, memLen);
      }
      static public unsafe void toDXTNDirect(byte[] coeffs, int numXBlocks, int numZBlocks, ref byte[] DXTN, ref int memLen)
      {
         byte* DXNData = null;
         CompressToDXTNDirect(coeffs, numXBlocks, numZBlocks, &DXNData, ref memLen);
         IntPtr p = new IntPtr(DXNData);
         DXTN = new byte[memLen];
         System.Runtime.InteropServices.Marshal.Copy(p, DXTN, 0, memLen);

      }

      public enum eTileCopyFormats
      {
         cTCFMT_R16F = 0,
         cTCFMT_R8G8 = 1,
         cTCFMT_DXN = 2,
         cTCFMT_L8 = 3,
         cTCFMT_DXT5A = 4,
         cTCFMT_R11G11B10 = 5,
         cTCFMT_DXT1 = 6,
         cTCFMT_G16R16 = 7,
      };



      static public unsafe void tileCopy(ref short[] dst, short[] src, int numXBlocks, int numZBlocks, eTileCopyFormats format)
      {
         int memSize = src.Length * sizeof(short);
         IntPtr shortArray = System.Runtime.InteropServices.Marshal.AllocHGlobal(memSize);
         System.Runtime.InteropServices.Marshal.Copy(src, 0, shortArray, src.Length);

         tileCopyData((void*)shortArray.ToPointer(), (void*)shortArray.ToPointer(), numXBlocks, numZBlocks, (int)format, -1);

         System.Runtime.InteropServices.Marshal.Copy(shortArray, dst, 0, src.Length);
         System.Runtime.InteropServices.Marshal.FreeHGlobal(shortArray);

      }
      static public unsafe void tileCopy(ref byte[] dst, byte[] src, int numXBlocks, int numZBlocks, int fmt)
      {
         int memSize = src.Length;

         IntPtr shortArray = System.Runtime.InteropServices.Marshal.AllocHGlobal(memSize);
         System.Runtime.InteropServices.Marshal.Copy(src, 0, shortArray, src.Length);

         bool ok = tileCopyData((void*)shortArray.ToPointer(), (void*)shortArray.ToPointer(), numXBlocks, numZBlocks, fmt, 0);

         System.Runtime.InteropServices.Marshal.Copy(shortArray, dst, 0, src.Length);
         System.Runtime.InteropServices.Marshal.FreeHGlobal(shortArray);

      }
      static public unsafe void tileCopy(ref Int32[] dst, Int32[] src, int numXBlocks, int numZBlocks)
      {
         int memSize = src.Length * 4;//sizeof(dword)

         IntPtr shortArray = System.Runtime.InteropServices.Marshal.AllocHGlobal(memSize);
         System.Runtime.InteropServices.Marshal.Copy(src, 0, shortArray, src.Length);

         tileCopyData((void*)shortArray.ToPointer(), (void*)shortArray.ToPointer(), numXBlocks, numZBlocks, (int)eTileCopyFormats.cTCFMT_R11G11B10, 0);

         System.Runtime.InteropServices.Marshal.Copy(shortArray, dst, 0, src.Length);
         System.Runtime.InteropServices.Marshal.FreeHGlobal(shortArray);

      }
      static public byte[] StructToByteArray(object _oStruct)
      {
         try
         {
            // This function copys the structure data into a byte[] 

            //Set the buffer ot the correct size 
            byte[] buffer = new byte[Marshal.SizeOf(_oStruct)];

            //Allocate the buffer to memory and pin it so that GC cannot use the 
            //space (Disable GC) 
            GCHandle h = GCHandle.Alloc(buffer, GCHandleType.Pinned);

            // copy the struct into int byte[] mem alloc 
            Marshal.StructureToPtr(_oStruct, h.AddrOfPinnedObject(), false);

            h.Free(); //Allow GC to do its job 

            return buffer; // return the byte[]. After all thats why we are here 
            // right. 
         }
         catch (Exception ex)
         {
            throw ex;
         }
      }

      #endregion



   }

   //---------------------------------------
   //---------------------------------------
   public class NetworkedAOGen
   {
      public const int cMajik = 0xA001;
      public void computeAOSection(string ouputDumpFilename,  int numSections, int mySecetion, Terrain.AmbientOcclusion.eAOQuality quality)
      {
         int numSamples = (int)quality;
         int samplesPerSection = (int)(numSamples / numSections);
         int startSampleCount = samplesPerSection * mySecetion;
         int endSampleCount = startSampleCount + samplesPerSection;

         Terrain.AmbientOcclusion ao = new Terrain.AmbientOcclusion();
         float totalTime = 0;
         float peelTime = 0;
         float gatherTime = 0;
         ao.calcualteAO(quality, true, ref totalTime, ref peelTime, ref gatherTime, startSampleCount, endSampleCount);
         ao.destroy();
         ao = null;

         //write our AO to a binary file

         FileStream s = File.Open(ouputDumpFilename, FileMode.OpenOrCreate, FileAccess.Write);
         BinaryWriter f = new BinaryWriter(s);
         int width = TerrainGlobals.getTerrain().getNumXVerts();
         int height = TerrainGlobals.getTerrain().getNumZVerts();

         //write our header.
         f.Write(cMajik);
         f.Write(numSections);
         f.Write(mySecetion);
         f.Write(numSamples);
         f.Write(startSampleCount);
         f.Write(endSampleCount);

         f.Write(width);
         f.Write(height);
         for (int x = 0; x < width; x++)
         {
            for (int y = 0; y < height; y++)
            {
               float AOVal = TerrainGlobals.getTerrain().getAmbientOcclusion(x, y);
               f.Write(AOVal);
            }
         }
         
         f.Close();
         s.Close();
      }

      public void buildAOFiles(string inputFilename, int numSections)
      {
         float []AOVals = null;
         for(int i=0;i<numSections;i++)
         {
            //work\scenario\development\coltTest\coltTest.AO0, .AO1, .AO2...., .AON
            string outFileName = Path.ChangeExtension(inputFilename, ".AO" + i);

            FileStream sr = File.Open(outFileName, FileMode.Open, FileAccess.Read);
            BinaryReader fr = new BinaryReader(sr);

            int majik = fr.ReadInt32();
            if (majik != cMajik)
            {
               AOVals = null;
               sr.Close();
               sr = null;
               fr.Close();
               fr = null;

               return;
            }

            int numSectionsT = fr.ReadInt32();
            Debug.Assert(numSectionsT == numSections);

            int mySecetion = fr.ReadInt32();
            int numSamples = fr.ReadInt32();
            int startSampleCount = fr.ReadInt32();
            int endSampleCount = fr.ReadInt32();
            int width = fr.ReadInt32();
            int height = fr.ReadInt32();

            if (AOVals == null)
               AOVals = new float[width *  height];

            for (int x = 0; x < width * height; x++)
            {
               AOVals[x] += fr.ReadSingle();
               Debug.Assert(AOVals[x] > 1);
            }

            sr.Close();
            sr = null;
            fr.Close();
            fr = null;
         }
         





         //now that we have our AO data read from file and accumulated, open our XTD and write over the previous AO Values.
         //first we have to get the file pointer offset from reading the file...
         int AOOffset = 0;
         int AOHeaderAdlerOffset = 0;
         ECFReader ecfR = new ECFReader();

         string XTDName = Path.ChangeExtension(inputFilename, ".XTD");
         if (!ecfR.openForRead(XTDName))
            return;

         int numXVerts =0;
         for(uint i=0;i<ecfR.getNumChunks();i++)
         {
            ECF.BECFChunkHeader chunkHeader = ecfR.getChunkHeader(i);
            eXTD_ChunkID id = (eXTD_ChunkID)chunkHeader.mID;
            switch(id)
            {
               case eXTD_ChunkID.cXTD_XTDHeader:
                  int version = Xbox_EndianSwap.endSwapI32(ecfR.readInt32());
                  numXVerts = Xbox_EndianSwap.endSwapI32(ecfR.readInt32());
                  break;

               case eXTD_ChunkID.cXTD_AOChunk:
                  AOHeaderAdlerOffset = (int)(i * ECF.BECFChunkHeader.giveSize() + ECF.ECFHeader.giveSize());
                  AOHeaderAdlerOffset += sizeof(Int64)*2;

                  AOOffset = chunkHeader.mOfs;

                  break;
            }
         }

         ecfR.close();
         ecfR = null;






         //now that we have our offset into the file, open it for write
         //generate our data
         byte[] data = XTDExporter.packAOToDXT5A(AOVals, numXVerts);

         FileStream s = File.Open(XTDName, FileMode.Open, FileAccess.ReadWrite);
         BinaryWriter f = new BinaryWriter(s);

         
         //reset the ADLER32 for this chunk..
         f.Seek(AOHeaderAdlerOffset, SeekOrigin.Begin);
         uint adler32 = (uint)Xbox_EndianSwap.endSwapI32((int)ECF.calcAdler32(data, 0, (uint)data.Length));
         f.Write(adler32);


         f.Seek(AOOffset, SeekOrigin.Begin);
         for (int i = 0; i < data.Length; i++)
            f.Write(data[i]);
         data = null;


         f.Close();
         f = null;
         s.Close();
         s = null;

         //DONE!
      }
   };

   //---------------------------------------

}
