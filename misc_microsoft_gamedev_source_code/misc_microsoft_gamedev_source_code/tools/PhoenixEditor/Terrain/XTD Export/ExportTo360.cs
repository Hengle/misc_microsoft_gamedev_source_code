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
   public class ExportSettings
   {
      private bool mRefineTerrain = true;
      public bool RefineTerrain
      {
         get
         {
            return mRefineTerrain;
         }
         set
         {
            mRefineTerrain = value;
         }
      }

      private float mRefineEpsilon = 0.1f;
      public float RefineEpsilon
      {
         get
         {
            return mRefineEpsilon;
         }
         set
         {
            mRefineEpsilon = value;
         }
      }

      private float mRefineBias = 0.75f;
      public float RefineMinorityBias
      {
         get
         {
            return mRefineBias;
         }
         set
         {
            mRefineBias = value;
         }
      }


      private AmbientOcclusion.eAOQuality mAmbientOcclusion = Terrain.AmbientOcclusion.eAOQuality.cAO_Off;
      public AmbientOcclusion.eAOQuality AmbientOcclusion
      {
         get
         {
            return mAmbientOcclusion;
         }
         set
         {
            mAmbientOcclusion = value;
         }
      }

      private bool mObjectsInAO = false;
      public bool ObjectsInAO
      {
         get
         {
            return mObjectsInAO;
         }
         set
         {
            mObjectsInAO = value;
         }
      }

      private int mUniqueTexRes = 2;
      public int UniqueTexRes
      {
         get
         {
            return mUniqueTexRes;
         }
         set
         {
            //CLM this should be set to a constant as defined by BTerrainTexturing
            //CLM COMMENTED OUT BECAUSE ARTISTS ARE STUPID AND FUCKING DON'T UNDERSTAND SHIT!
            mUniqueTexRes = 2;// (int)BMathLib.Clamp(value, 0, 5);
         }
      }

      private int mNavMeshQuantizationDist = 256;
      public int NavMeshQuantizationDist
      {
         get
         {
            return mNavMeshQuantizationDist;
         }
         set
         {
            mNavMeshQuantizationDist = value;
         }
      }

      private bool mNavMesh = false;
      public bool NavMesh
      {
         get
         {
            return mNavMesh;
         }
         set
         {
            mNavMesh = value;
         }
      }

      private eDXTQuality mExportTextureCompressionQuality = eDXTQuality.cDXTQualityLowest;
      public eDXTQuality ExportTextureCompressionQuality
      {
         get
         {
            return mExportTextureCompressionQuality;
         }
         set
         {

               mExportTextureCompressionQuality = value;
         }
      }

      public void SettingsQuick()
      {
         RefineTerrain = true;
         ObjectsInAO = false;
         AmbientOcclusion = Terrain.AmbientOcclusion.eAOQuality.cAO_Off;
      }
      public void SettingsFinal()
      {
         RefineTerrain = true;
         AmbientOcclusion = Terrain.AmbientOcclusion.eAOQuality.cAO_Final;
         ObjectsInAO = true;
         UniqueTexRes = 3;
      }
      public void copyTo(ref ExportSettings dest)
      {
         dest.UniqueTexRes = UniqueTexRes;
         dest.RefineTerrain = RefineTerrain;
         dest.RefineEpsilon = RefineEpsilon;
         dest.RefineMinorityBias = RefineMinorityBias;
         dest.AmbientOcclusion = AmbientOcclusion;
         dest.NavMesh = NavMesh;
         dest.NavMeshQuantizationDist = NavMeshQuantizationDist;
         dest.ExportTextureCompressionQuality = ExportTextureCompressionQuality;
      }
   }
   //---------------------------------------
   //---------------------------------------
   public class ExportResults
   {

      public int terrainPositionMemorySize = 0;
      public int terrainNormalsMemorySize = 0;
      public int terrainAOMemorySize = 0;
      public double terrainAOTime = 0;

      public int terrainLightingMemorySize = 0;
      public double terrainLightingTime = 0;

      public int terrainAlphaMemorySize = 0;
      public int terrainTessValuesMemorySize = 0;


      public int terrainTextureMemorySize = 0;
      public double terrainTextureTime = 0;
      public int terrainUniqueTextureMemorySize = 0;
      public double terrainUniqueTextureTime = 0;

      public int terrainSimMemorySize = 0;
      public double terrainSimTime = 0;


      public double terrainRoadTime = 0;
      public int terrainRoadMemory = 0;

      public double terrainFoliageTime = 0;
      public int terrainFoliageMemory = 0;

      public int terrainGPUHeighfieldMemory = 0;
      public double terrainGPUHeighfieldTime = 0;

      public double totalTime = 0;

      public int getTotalMemory()
      {
         return   terrainPositionMemorySize+
                  terrainNormalsMemorySize +
                  terrainAOMemorySize +
                  terrainLightingMemorySize +
                  terrainAlphaMemorySize +
                  terrainTessValuesMemorySize +
                  terrainTextureMemorySize+
                  terrainUniqueTextureMemorySize +
                  terrainSimMemorySize +
                  terrainRoadMemory +
                  terrainFoliageMemory + 
                  terrainGPUHeighfieldMemory;
      }
   }


   //---------------------------------------
   //---------------------------------------
   public class ExportTo360
   {
      //Generals
      static public DEP mDEP = new DEP();
      static public ECFWriter mECF = new ECFWriter();
      
      //dialog
      static public ExportProgressDialog mEPB = null;
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


      static public void destroy()
      {
         mEPB.Close();
         mEPB.Dispose();
         mEPB = null;

         if (mDEP != null) mDEP = null;
         if (mECF != null) { mECF.destroy(); mECF = null; }


         checkUnloadRender();
         mOutputOverrideDir = null;
      }
      static public void init()
      {
         if (mDEP == null) mDEP = new DEP();
         if (mECF == null) mECF = new ECFWriter();

         if (mEPB == null)
         {
            mEPB = new ExportProgressDialog();

         }
         mOutputOverrideDir = null;
      }



      //functions
      static public bool doExport(String baseFilename, ref ExportSettings expSettings, ref ExportResults results)
      {
         return doExport(baseFilename, baseFilename, null,ref expSettings, ref results, true, true, true, true, true, true, true, false);
      }
      static public bool doExport(String terrainFileName, string scenarioFileName, ref ExportSettings expSettings, ref ExportResults results)
      {
         return doExport(terrainFileName, scenarioFileName, null,ref expSettings, ref results, true, true, true, true, true, true, true, false);
      }
      static public bool doExport(String baseFilename,
                                             ref ExportSettings expSettings, ref ExportResults results,
                                             bool doXTD, bool doXTT, bool doXTH, bool doXSD, bool doLRP, bool doDEP, bool doTAG, bool doXMB)
      {
         return doExport(baseFilename, baseFilename, null,ref expSettings, ref results, doXTD, doXTT, doXTH, doXSD, doLRP, doDEP, doTAG, doXMB);
      }

      static public bool doExport(  String terrainFileName,
                                    String scenarioFileName,
                                    String outputOverrideDir,
                                             ref ExportSettings expSettings, ref ExportResults results,
                                             bool doXTD, bool doXTT, bool doXTH, bool doXSD, bool doLRP, bool doDEP, bool doTAG, bool doXMB)
      {
         //This is now done outside using work topics
         //if (CoreGlobals.UsingPerforce == true)
         //{
         //   if (!checkPerforceFileAlloances(terrainFileName, scenarioFileName,
         //                                 doXTD, doXTT, doXTH, doXSD, doLRP, doDEP, doTAG, doXMB))
         //      return false;
         //}

         init();
         int maxValues = (doXTD?1:0)  +  
                         (doXTT?1:0)  +  
                         (doXTH?1:0)  +  
                         (doXSD?1:0)  +
                         (doLRP?1:0)  +
                         (doDEP?1:0)  +
                         (doTAG?1:0)  +  
                         (doXMB?1:0)  +
                         (expSettings.AmbientOcclusion!= AmbientOcclusion.eAOQuality.cAO_Off?1:0)
            ;

         initOverrideDir(outputOverrideDir);
         

         mEPB.Init(maxValues);
         mEPB.setName(Path.GetFileNameWithoutExtension(terrainFileName));
         mEPB.Show();
         DateTime start = DateTime.Now;
        

         GC.Collect();
         GC.WaitForPendingFinalizers();


         mECF.clear();
         mDEP.clear();

         bool sofarSoGood = true;

         

         if (sofarSoGood && doXTT)
         {
            XTTExporter XTTexporter = new XTTExporter();
            sofarSoGood &= XTTexporter.export_XTT(terrainFileName, expSettings, ref results);
            XTTexporter.destroy();
            XTTexporter = null;
            ExportTo360.mECF.clear();
            ExportProgressDialog.Increase();
         }



         //From here on out we don't need textures in memory..
         if (sofarSoGood && (expSettings.AmbientOcclusion != AmbientOcclusion.eAOQuality.cAO_Off || doXTH))
         {
            BRenderDevice.getTextureManager().freeAllD3DTextureHandles();
         }



         if (sofarSoGood && doXTD)
         {    
            XTDExporter XTDexporter = new XTDExporter();
            sofarSoGood &= XTDexporter.export_XTD(terrainFileName, expSettings, ref results);
            XTDexporter.destroy();
            XTDexporter = null;
            ExportTo360.mECF.clear();
            ExportProgressDialog.Increase();
         }



         if (sofarSoGood && doXSD)
         {
            XSDExporter XSDexporter = new XSDExporter();
            sofarSoGood &= XSDexporter.export_XSD(scenarioFileName, expSettings, ref results);
            XSDexporter.destroy();
            if(sofarSoGood && doLRP)
            {
               sofarSoGood &= XSDexporter.CreateLRP(spliceFilenameWithOverride(scenarioFileName));
            }
            XSDexporter = null;
            ExportTo360.mECF.clear(); 
            ExportProgressDialog.Increase();
         }


         if (sofarSoGood && doXTH)
         {
            XTHExporter XTHexporter = new XTHExporter();
            sofarSoGood &= XTHexporter.export_XTH(terrainFileName, expSettings, ref results);
            XTHexporter.destroy();
            XTHexporter = null;
            ExportTo360.mECF.clear();
            ExportProgressDialog.Increase();
         }


         // OVERRIDE! if ANY terrain files are changed, then generate a .TAG
         if (sofarSoGood && doTAG && (doXTT | doXTD | doXTH | doXSD))
         {
            TAG mTAG = new TAG();
            String[] extentions = new String[] { ".XTD", ".XTT", ".XTH", ".XSD", ".LRP" };
            String desiredFilename = "";
            for (int i = 0; i < extentions.Length-1; i++)
            {
               desiredFilename = spliceFilenameWithOverride(Path.ChangeExtension(terrainFileName, extentions[i]));
               mTAG.addFile(desiredFilename);
            }

            //XSD
            desiredFilename = spliceFilenameWithOverride(Path.ChangeExtension(scenarioFileName, extentions[extentions.Length - 2]));
            mTAG.addFile(desiredFilename);
            desiredFilename = spliceFilenameWithOverride(Path.ChangeExtension(scenarioFileName, extentions[extentions.Length - 1]));
            mTAG.addFile(desiredFilename);

            mTAG.writeToFile(spliceFilenameWithOverride(Path.ChangeExtension(scenarioFileName, ".TAG")));
            mTAG = null;


            ExportProgressDialog.Increase();
         }





         if (sofarSoGood && doXTT && doDEP) //CLM XTT is the only one that marks dependents currently...
         {
            mDEP.writeToFile(spliceFilenameWithOverride(terrainFileName));
         }
         



         if(sofarSoGood && doXMB)
         {
            string outputDir = null;
            if (mOutputOverrideDir != null)
            {
               outputDir = spliceFilenameWithOverride(terrainFileName);
               outputDir = Path.GetDirectoryName(outputDir);
            }

            if (File.Exists(Path.ChangeExtension(scenarioFileName, ".SCN")))
               XMBProcessor.CreateXMB(Path.ChangeExtension(scenarioFileName, ".SCN"), outputDir, false);
            if (File.Exists(Path.ChangeExtension(scenarioFileName, ".GLS")))
               XMBProcessor.CreateXMB(Path.ChangeExtension(scenarioFileName, ".GLS"), outputDir, false);

            if (File.Exists(Path.ChangeExtension(scenarioFileName, ".DEP")))  
               XMBProcessor.CreateXMB( spliceFilenameWithOverride(Path.ChangeExtension(scenarioFileName, ".DEP")), false);
            ExportProgressDialog.Increase();
         }




         TimeSpan ts = DateTime.Now - start;
         results.totalTime = ts.TotalMinutes;

         destroy();

         GC.Collect();
         GC.WaitForPendingFinalizers();

         //if we've cleared the textures, force them to reload..
         if (expSettings.AmbientOcclusion != AmbientOcclusion.eAOQuality.cAO_Off || doXTH)
         {
            BRenderDevice.getTextureManager().reloadTexturesIfNeeded(true);
         }

         mOutputOverrideDir = null;

         return sofarSoGood;

      }


      private static bool P4CanEditFile(string filename)
      {
         if (CoreGlobals.UsingPerforce == false)
         {
            throw new System.Exception("Editor Bug.  Invalid perforce use");
         }

         SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(filename);
         if (status.InPerforce == false) return true;
         if(!status.CheckedOut)
         {
            CoreGlobals.ShowMessage(Path.GetFileName(filename) + " is not checked out from perforce. Export Aborted!");
            return false;
         }
         if (status.CheckedOutOtherUser == true)
         {
            CoreGlobals.ShowMessage(Path.GetFileName(filename) + " is checked out by " + status.ActionOwner + ". Export Aborted!");
            return false;
         }

         return true;
      }

      static bool checkSafeToWrite(string desiredFilename)
      {
         if (File.Exists(desiredFilename))
         {
            if (CoreGlobals.UsingPerforce == true)
            {
               if (P4CanEditFile(desiredFilename))
               {
                  if (File.GetAttributes(desiredFilename) == FileAttributes.ReadOnly)
                  {
                     if (!CoreGlobals.IsBatchExport)
                        CoreGlobals.ShowMessage("ERROR File checked out, but still set as 'READ ONLY': " + desiredFilename);
                     return false;
                  }
                  return true;
               }
               else
               {
                  CoreGlobals.ShowMessage("ERROR Cannot export. Please check out: " + desiredFilename);
                  return false;
               }
            }
            else
            {
               if ((File.GetAttributes(desiredFilename) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
               {
                  CoreGlobals.ShowMessage("ERROR file is set as 'READ ONLY': " + desiredFilename);
                  return false;
               }
               else
               {
                  return true;
               }
            }
         }
         return false;
      }
      static public bool checkPerforceFileAlloances(String terrainName, String scenarioName,
                                    bool doXTD, bool doXTT, bool doXTH, bool doXSD, bool doLRP, bool doDEP, bool doTAG, bool doXMB)
      {


         if (doXTD)
         {
            if (!P4CanEditFile(Path.ChangeExtension(terrainName, ".XTD"))) return false;

         }
         if (doXTT)
         {
            if (!P4CanEditFile(Path.ChangeExtension(terrainName, ".XTT"))) return false;
            if (!P4CanEditFile(Path.ChangeExtension(terrainName, ".DEP"))) return false;
         }
         if (doXTH)
         {
            if (!P4CanEditFile(Path.ChangeExtension(terrainName, ".XTH"))) return false;
         }
         if (doXSD)
         {
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".XSD"))) return false;
         }
         if (doLRP)
         {
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".LRP"))) return false;
         }

         if (doDEP)
         {
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".DEP"))) return false;
         }

         if (doTAG)
         {
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".TAG"))) return false;
         }

         if (doXMB)
         {
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".SCN.XMB"))) return false;
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".GLS.XMB"))) return false;
            if (!P4CanEditFile(Path.ChangeExtension(scenarioName, ".DEP.XMB"))) return false;
         }

         return true;
      }




      static public void initOverrideDir(string outputOverrideDir)
      {
         if (outputOverrideDir == null)
            return;

         mOutputOverrideDir = outputOverrideDir;
         //persistance check..
         if (mOutputOverrideDir[mOutputOverrideDir.Length - 1] != '\\')
            mOutputOverrideDir += "\\";

         ensurePathExists(mOutputOverrideDir);
      }
      static public string spliceFilenameWithOverride(String filename)
      {
         if (mOutputOverrideDir == null)
            return filename;

         string rootWorkDir = CoreGlobals.getWorkPaths().mGameDirectory;
         if (!filename.Contains(rootWorkDir))
            return filename;  //it's a relative path.. don't fuck with it..

         string res = filename.Substring(rootWorkDir.Length);
         if (res[0] == '\\')
            res = res.Substring(1);

        // string res = Path.GetFileName(filename);
         return mOutputOverrideDir + res;
      }
      static public void ensurePathExists(string path)
      {
         if (!File.Exists(path))
            System.IO.Directory.CreateDirectory(path);
      }
      static public bool safeECFFileWrite(String filename, String ext)
      {

         String desiredFilename = spliceFilenameWithOverride(Path.ChangeExtension(filename, ext));
         String tempDesiredFilename = desiredFilename;

         ensurePathExists(Path.GetDirectoryName(desiredFilename));

         //now that we have all the chunks, write the XTT to file.
         tempDesiredFilename = Path.ChangeExtension(tempDesiredFilename, ext + "_");
         ExportTo360.mECF.writeToFile(tempDesiredFilename);

         try
         {
               //NOW RENAME & DELETE TO ENSURE GOOD WRITES ALL THE TIME
               if (File.Exists(desiredFilename))
                  File.Delete(desiredFilename);

               FileInfo fi = new FileInfo(tempDesiredFilename);
               fi.MoveTo(desiredFilename);
               File.Delete(tempDesiredFilename);
            }
            catch (System.UnauthorizedAccessException ex)
            {
               if(!CoreGlobals.IsBatchExport)
                  CoreGlobals.ShowMessage("Error saving " + desiredFilename + ".\n Make sure this file is checked out from perforce, and that it is not open by XFS. \n EXPORT ABORTED");
               return false;
            }

         return true;
      }




      static public void addTextureChannelDependencies(String inFilename)
      {
         String filename = inFilename;
         String[] extentionTypes = new String[] { "_df", "_nm", "_sp", "_em", "_rm", "_op" };

         for (int i = 0; i < extentionTypes.Length; i++)
         {
            if (Path.GetFileNameWithoutExtension(inFilename).LastIndexOf(extentionTypes[i]) != -1)
            {
               filename = filename.Substring(0, filename.LastIndexOf(extentionTypes[i]));
               break;
            }
         }

         //Remove the "_" if it exists in the filename..
         //if(Path.GetFileNameWithoutExtension(inFilename).LastIndexOf('_')!=-1)
         //   filename = filename.Substring(0, filename.LastIndexOf('_'));
         
         //remove extention if it exists in the filename
         if (Path.GetExtension(filename) != "")
            filename = Path.ChangeExtension(filename, "");

         
         for(int i=0;i<extentionTypes.Length;i++)
         {
            String tPath = filename + extentionTypes[i] + ".ddx";
            if (File.Exists(tPath)) 
               mDEP.addFileDependent(tPath, i!=0);

         }
      }


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
