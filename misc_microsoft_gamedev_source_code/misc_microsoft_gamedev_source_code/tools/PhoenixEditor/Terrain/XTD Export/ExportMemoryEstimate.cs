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
using SimEditor;

/*
 * CLM 12.16.07
 * 
 * 
 */

//--------------------
namespace Export360
{
   //---------------------------------------
   //---------------------------------------
   public class ExportMemoryEstimate
   {
      public enum eMainCatagory
      {
         eCat_Terrain = 0,
         eCat_Sim,
      }
      public class memoryElement
      {
         public memoryElement(string name, int memSize, eMainCatagory cat)
         {
            mName = name;
            mMemoryInBytes = memSize;
            mCatagory = cat;
         }
         public int mMemoryInBytes;
         public string mName;
         public eMainCatagory mCatagory;
      };
      
      static int mMaxTerrainMemory = 52428800; //50mb
      static int mMaxModelMemory = 138412032; // 100mb for textures, 32mb for UGX/UAX

      //---------------------------------------
      static int mEstimatingMemory = 0;   //we can only do one estimate at a time..
      static public bool isCalculatingMemoryEstimate()
      {
         return mEstimatingMemory==1;
      }
      //---------------------------------------
      

      static List<memoryElement> mMemoryUsageItems = new List<memoryElement>();
      static int findMemoryElement(string name)
      {
         for (int i = 0; i < mMemoryUsageItems.Count; i++)
            if (mMemoryUsageItems[i].mName.Equals(name))
               return i;
         return -1;
      }
      static void setOrAddMemoryElement(string name, int memSize, eMainCatagory cat)
      {
         int ex = findMemoryElement(name);
         if(ex==-1)
         {
            ex = mMemoryUsageItems.Count;
            mMemoryUsageItems.Add(new memoryElement(name, memSize, cat));
         }

         mMemoryUsageItems[ex].mMemoryInBytes = memSize;
      }
      //---------------------------------------
      #region GENERAL METHODS

      static public int giveTotalMaxMemory()
      {
         return mMaxTerrainMemory + mMaxModelMemory;
      }
      static public int giveTotalMaxMemoryCatagory(eMainCatagory cat)
      {
         if (cat == eMainCatagory.eCat_Terrain)
            return mMaxTerrainMemory;
         else if (cat == eMainCatagory.eCat_Sim)
            return mMaxModelMemory;

         return 0;
      }
      static public int giveTotalAvailableMemory()
      {
         int sum = giveTotalMaxMemory() - giveTotalMemoryUsage();
         if (sum < 0) sum = 0;
         return sum;
      }
      static public int giveTotalAvailableMemoryCatagory(eMainCatagory cat)
      {
         int sum = giveTotalMaxMemoryCatagory(cat) - giveTotalMemoryUsageCatagory(cat);
         if (sum < 0) sum = 0;
         return sum;
      }

      static public int giveTotalMemoryUsage()
      {
         int sum = 0;
         for (int i = 0; i < mMemoryUsageItems.Count; i++)
         {
            sum += mMemoryUsageItems[i].mMemoryInBytes;
         }
         return sum;
      }
      static public int giveTotalMemoryUsageCatagory(eMainCatagory cat)
      {
         int sum = 0;
         for (int i = 0; i < mMemoryUsageItems.Count; i++)
         {
            if (mMemoryUsageItems[i].mCatagory == cat)
               sum += mMemoryUsageItems[i].mMemoryInBytes;
         }
         return sum;
      }

      static public float giveMemoryUsagePercent()
      {
         int sum = 0;
         for (int i = 0; i < mMemoryUsageItems.Count; i++)
         {
            sum += mMemoryUsageItems[i].mMemoryInBytes;
         }

         float perc = 0;
         perc = sum / (float)giveTotalMaxMemory();
         return perc;
      }
      static public float giveMemoryUsageCatagoryPercent(eMainCatagory cat)
      {
         int sum = 0;
         for (int i = 0; i < mMemoryUsageItems.Count; i++)
         {
            if (mMemoryUsageItems[i].mCatagory == cat)
               sum += mMemoryUsageItems[i].mMemoryInBytes;
         }

         float perc = 0;
         if (cat == eMainCatagory.eCat_Terrain)
            perc = sum / (float)mMaxTerrainMemory;
         else if (cat == eMainCatagory.eCat_Sim)
            perc = sum / (float)mMaxModelMemory;

         return perc;
      }

      //---------------------------------------
      static public int getNumMemoryElements()
      {
         return mMemoryUsageItems.Count;
      }
      static public memoryElement getMemoryElement(int index)
      {
         if (index < 0 || index >= mMemoryUsageItems.Count)
            return null;

         return mMemoryUsageItems[index];
      }
#endregion
      //---------------------------------------
      #region SIM ESTIMATE
      static string fName = "_MemEstExp"; 

      static void estimateSimMemory(bool forceAutoSave)
      {
         using (PerfSection p3 = new PerfSection("estimateSimMemory"))
         {

            //   BVisualManager.BModelStatistics stats = BVisualManager.giveTotal360MemoryEstimate();
            //   int texMem = GrannyManager2.giveTotal360TextureMemoryEstimate();

            //DO a quicksave, run arcGen to generate the ENTIRE file list that could exist, and use that info...
            quickSaveFile(forceAutoSave);

            generateFileList(fName);

            int UGXMem = 0, UAXMem = 0, DDXMem = 0;
            //file will be in form of "<filename>_fileList.txt" : _memestsave_FileList.txt
            processFileList(fName + "_FileList.txt", out UGXMem, out UAXMem, out DDXMem);
            quickDeleteFile(fName);

            setOrAddMemoryElement("Model UGX Memory", UGXMem, eMainCatagory.eCat_Sim);
            setOrAddMemoryElement("Model UAX Memory", UAXMem, eMainCatagory.eCat_Sim);
            setOrAddMemoryElement("Model Texture Memory", DDXMem, eMainCatagory.eCat_Sim);

            System.Threading.Interlocked.Exchange(ref mEstimatingMemory, 0);

         }
      }
      static void quickSaveFile(bool forceAutoSave)
      {
         using (PerfSection p3 = new PerfSection("quickSaveFile"))
         {
            string scenerioPath = CoreGlobals.getWorkPaths().mGameScenarioDirectory;
            string scenarioName = scenerioPath + @"\" + fName + @"\" + fName + ".scn";
            string terrainName = Path.ChangeExtension(scenarioName, "ted");
            string lightsetName = Path.ChangeExtension(scenarioName, "gls");


            if (forceAutoSave || !File.Exists(scenarioName))
            {
               if (!Directory.Exists(scenerioPath + @"\" + fName))
                  Directory.CreateDirectory(scenerioPath + @"\" + fName);

               XMBProcessor.Pause();
               SimGlobals.getSimMain().SaveScenario(scenarioName, terrainName, lightsetName, true);
               SimGlobals.getSimMain().SaveExtasMacro(scenarioName);
               XMBProcessor.Unpause();
            }
         }
      }
      static void quickDeleteFile(string fName)
      {
         if (Directory.Exists("cache"))
            Directory.Delete("cache", true);

         if (File.Exists(fName + "_FileList.txt"))
            File.Delete(fName + "_FileList.txt");

         string scnNam = CoreGlobals.getWorkPaths().mGameScenarioDirectory + @"\" + fName;
         if (Directory.Exists(scnNam))
            Directory.Delete(CoreGlobals.getWorkPaths().mGameScenarioDirectory + @"\" + fName, true);
         
      }

      static void generateFileList(string fName)
      {
         using (PerfSection p3 = new PerfSection("generateFileList"))
         {

            if (!Directory.Exists("cache"))
               Directory.CreateDirectory("cache");

            string scenerioPath = CoreGlobals.getWorkPaths().mGameScenarioDirectory;
            string scenarioName = scenerioPath + @"\" + fName + @"\" + fName + ".scn";

            string arguments = " -quiet";
            arguments = arguments + @" -scriptfile " + CoreGlobals.getWorkPaths().mBaseDirectory + @"\settings\MemEst_createScenarioFileList.xml";
            arguments = arguments + @" /define scenariofilename=" + scenarioName;

            string argGenPath = CoreGlobals.getWorkPaths().mToolsDirectory + @"\arcGen\arcGen.exe";
            System.Diagnostics.Process arcgenUtility = new System.Diagnostics.Process();
            arcgenUtility.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
            //arcgenUtility.StartInfo.CreateNoWindow = true;
            arcgenUtility.StartInfo.Arguments = arguments;
            arcgenUtility.StartInfo.FileName = argGenPath;
            arcgenUtility.Start();

            arcgenUtility.WaitForExit();
            if (arcgenUtility.ExitCode != 0)
            {
               //!!!!!
            //   CoreGlobals.ShowMessage("Error calculating memory estimates for : " + scenarioName + "\n Please send a screenshot of this message to the editor programmers.");
            }

            arcgenUtility.Close();
         }
      }
      static void processFileList(string listfilename, out int UGXMem, out int UAXMem, out int DDXMem)
      {
         using (PerfSection p3 = new PerfSection("processFileList"))
         {
            UGXMem = 0;
            UAXMem = 0;
            DDXMem = 0;

            if (!File.Exists(listfilename))
               return;

            Stream st = null;
            // if we're in async mode, we may not have access to this yet.
            while (st == null)
            {
               try
               {
                  st = File.OpenRead(listfilename);
               }
               catch (IOException e)
               {

               }
            }

            StreamReader tr = new StreamReader(st);
            int otherFiles = 0;

            List<string> ddxFiles = new List<string>();

            try
            {
               string filename = tr.ReadLine();
               do
               {
                  if (!File.Exists(filename))
                     continue;

                  if (filename.ToLower().Contains(".xmb"))
                     continue;
                  if (filename.ToLower().Contains(".lgt"))
                     continue;
                  if (filename.ToLower().Contains(".tfx"))
                     continue;
                  if (filename.ToLower().Contains(".xpr"))
                     continue;
                  if (filename.ToLower().Contains(".scn"))
                     continue;
                  if (filename.ToLower().Contains(".txt"))
                     continue;
                  if (filename.ToLower().Contains(".gls"))
                     continue;

                  if (filename.ToLower().Contains(".ddx"))
                  {

                     ddxFiles.Add(filename);
                     //DDXMem += DDXBridge.give360TextureMemFootprint(filename);

                  }
                  else if (filename.ToLower().Contains(".ugx"))
                  {

                     System.IO.FileInfo fi = new System.IO.FileInfo(filename);
                     UGXMem += (int)fi.Length;
                     fi = null;

                  }
                  else if (filename.ToLower().Contains(".uax"))
                  {

                     System.IO.FileInfo fi = new System.IO.FileInfo(filename);
                     UAXMem += (int)fi.Length;
                     fi = null;
                  }
                  else
                  {
                     otherFiles++;
                  }

               } while ((filename = tr.ReadLine()) != null);

               //using (PerfSection p4 = new PerfSection("give360TextureMemFootprint"))
               {
                  foreach (string s in ddxFiles)
                  {
                     DDXMem += DDXBridge.give360TextureMemFootprint(s);

                  }
               }

            }
            catch (IOException e)
            {

            }
            finally
            {
               tr.Close();
               st.Close();
            }
         }
      }

      #endregion
      //---------------------------------------------------
      #region TERRAIN ESTIMATE
      static int giveTextureCacheMemoryRequirement(int numCachePages, int mip0Width, int numMips, int fmt)
      {
         int bitsPerPixel = 0;
         if (fmt == 0) //DXT1
            bitsPerPixel = 4;
         if (fmt == 1) //DXN / DXT5
            bitsPerPixel = 8;

         int mip0Size = (numCachePages * (mip0Width * mip0Width) * bitsPerPixel) >> 3;

         int mipChainSize = 0;
         for (int R = 1; R < numMips; R++)
            mipChainSize += mip0Size >> (2 * R);

         return mip0Size + mipChainSize;
      }
      static void estimateTerrainMemory()
      {
         using (PerfSection p3 = new PerfSection("estimateTerrainMemory"))
         {

            int width = TerrainGlobals.getTerrain().getNumXVerts();

            //CLM THESE ESTIMATES ARE VALID AS OF [07.24.07]


            ///XTD
            //positions & normals - each component is 32bit
            mMemoryUsageItems.Add(new memoryElement("Terrain Positions & Normals", (width * width * 8), eMainCatagory.eCat_Terrain));

            //ALPHA - DXT5A
            mMemoryUsageItems.Add(new memoryElement("Terrain Alpha", ((width >> 2) * (width >> 2) * 8), eMainCatagory.eCat_Terrain));

            //AO - DXT5A
            mMemoryUsageItems.Add(new memoryElement("Terrain AO", ((width >> 2) * (width >> 2) * 8), eMainCatagory.eCat_Terrain));

            //TessData
            int nodeWidth = 16;
            int numXPatches = (int)(TerrainGlobals.getTerrain().getNumXVerts() / (float)nodeWidth);
            int tessMem = numXPatches * numXPatches;
            tessMem += (numXPatches * numXPatches * 2) * (sizeof(float) * 4);
            mMemoryUsageItems.Add(new memoryElement("Terrain Tessellation", tessMem, eMainCatagory.eCat_Terrain));

            //light data - DXT1
            if (LightManager.hasTerrainLightData())
               mMemoryUsageItems.Add(new memoryElement("Terrain Precomputed Lighting", ((width * width) >> 2), eMainCatagory.eCat_Terrain));
             
             
            //////XTT
            //texture alpha data
            int texAlphaMem = 0;
            BTerrainQuadNode[] nodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
            for (int i = 0; i < nodes.Length; i++)
            {
               int numAlignedSplatLayers = 4 * (((nodes[i].mLayerContainer.getNumSplatLayers() - 1) >> 2) + 1);
               int numAlignedDecalLayers = 4 * (((nodes[i].mLayerContainer.getNumDecalLayers() - 1) >> 2) + 1);

               texAlphaMem += numAlignedSplatLayers * ((64 * 64) >> 2);
            }
            mMemoryUsageItems.Add(new memoryElement("Terrain Texture Blends", texAlphaMem, eMainCatagory.eCat_Terrain));

            //unique Albedo
            int uniqueTextureRes = 128;
            int uniqueWidth = (int)((width / BTerrainQuadNode.cMaxWidth) * uniqueTextureRes);
            int uniqueMemSize = (int)(uniqueWidth * uniqueWidth);

            if (BMathLib.isPow2(uniqueWidth))
               uniqueMemSize += (uniqueWidth >> 1) * (uniqueWidth >> 1);

            mMemoryUsageItems.Add(new memoryElement("Terrain Unique Albedo Texture", (uniqueMemSize >> 1), eMainCatagory.eCat_Terrain));

            //FOLIAGE
            int totalPhysicalMemory = 0;
            for (int qi = 0; qi < FoliageManager.getNumChunks(); qi++)
            {
               FoliageQNChunk chunk = FoliageManager.getChunk(qi);
               for (int setI = 0; setI < chunk.mSetsUsed.Count; setI++)
               {
                  int numBlades = 0;
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
                           totalPhysicalMemory += (fs.mNumVertsPerBlade + 1) * sizeof(int);
                        }
                     }
                  }
               }
            }
            mMemoryUsageItems.Add(new memoryElement("Terrain Foliage", totalPhysicalMemory, eMainCatagory.eCat_Terrain));


            //ROADS

            /////XTH
            mMemoryUsageItems.Add(new memoryElement("Terrain Decal Rep", 256 * 256 * sizeof(short) * 2, eMainCatagory.eCat_Terrain));


            //CACHES
            //add in our 360 cache data
            int cacheMemCount = 0;
            const int numCachePages = 20;
            const int cachePageSize = 512;
            const int numMips = 2;
            bool albedoCache = true;
            bool normalCache = true;
            bool specCache = false;
            bool selfCache = false;
            bool envCache = false;

            for (int i = 0; i < TerrainGlobals.getTexturing().getActiveTextureCount(); i++)
            {
               BTerrainActiveTextureContainer ActTex = TerrainGlobals.getTexturing().getActiveTexture(i);

               String fname = ActTex.mFilename;

               String ext = ".ddx";// Path.GetExtension(fname);

               string sname = fname.Substring(0, fname.LastIndexOf("_df")) + "_em" + ext;
               selfCache |= File.Exists(sname);
               sname = fname.Substring(0, fname.LastIndexOf("_df")) + "_rm" + ext;
               envCache |= File.Exists(sname);
               sname = fname.Substring(0, fname.LastIndexOf("_df")) + "_sp" + ext;
               specCache |= File.Exists(sname);
            }
            for (int i = 0; i < TerrainGlobals.getTexturing().getActiveDecalCount(); i++)
            {
               BTerrainActiveDecalContainer ActTex = TerrainGlobals.getTexturing().getActiveDecal(i);

               String fname = ActTex.mFilename;

               String ext = ".ddx";// Path.GetExtension(fname);

               string sname = fname.Substring(0, fname.LastIndexOf("_df")) + "_em" + ext;
               selfCache |= File.Exists(sname);
               sname = fname.Substring(0, fname.LastIndexOf("_df")) + "_rm" + ext;
               envCache |= File.Exists(sname);
               sname = fname.Substring(0, fname.LastIndexOf("_df")) + "_sp" + ext;
               specCache |= File.Exists(sname);
            }

            if (albedoCache) cacheMemCount += giveTextureCacheMemoryRequirement(numCachePages, cachePageSize, numMips, 0);   //DXT1 * numCachePages (mip0 & mip1)
            if (normalCache) cacheMemCount += giveTextureCacheMemoryRequirement(numCachePages, cachePageSize, numMips, 1); ;//DXN
            if (specCache) cacheMemCount += giveTextureCacheMemoryRequirement(numCachePages, cachePageSize, numMips, 0);  //DXT1
            if (envCache) cacheMemCount += giveTextureCacheMemoryRequirement(numCachePages, cachePageSize, numMips, 0);   //DXT1
            if (selfCache) cacheMemCount += giveTextureCacheMemoryRequirement(numCachePages, cachePageSize, numMips, 1);  //DXT5
            mMemoryUsageItems.Add(new memoryElement("Terrain Texture Cache", cacheMemCount, eMainCatagory.eCat_Terrain));


            //ARTIST TERRAIN TEXTURES
            int totalMemTT = 0;
            for (int i = 0; i < TerrainGlobals.getTexturing().getActiveTextureCount(); i++)
               totalMemTT += TerrainGlobals.getTexturing().getActiveTexture(i).m360MemoryFootprint;
            for (int i = 0; i < TerrainGlobals.getTexturing().getActiveDecalCount(); i++)
               totalMemTT += TerrainGlobals.getTexturing().getActiveDecal(i).m360MemoryFootprint;
            mMemoryUsageItems.Add(new memoryElement("Terrain Artist Textures", totalMemTT, eMainCatagory.eCat_Terrain));
         }
      }
      #endregion
      //---------------------------------------------------
      static public void estimateMemory(bool forceAutoSave,bool asyncTest)
      {
         if (mEstimatingMemory!=0)
            return;

         using (PerfSection p3 = new PerfSection("estimateMemory"))
         {


            System.Threading.Interlocked.Exchange(ref mEstimatingMemory, 1);

            TerrainGlobals.getTerrainFrontEnd().setMemoryEstimateString(null, "Calculating..");
            if (asyncTest)
            {
               int hackedStatePacket = forceAutoSave ? 1 : 0;
               hackedStatePacket = hackedStatePacket << 1;
               hackedStatePacket |= asyncTest ? 1 : 0;

               BackgroundWorker mWorkerThread = new BackgroundWorker();
               mWorkerThread.WorkerReportsProgress = false;
               mWorkerThread.WorkerSupportsCancellation = true;
               mWorkerThread.DoWork += bw_Work;
               mWorkerThread.RunWorkerAsync(hackedStatePacket);

            }
            else
            {
               doMemoryEstimate(forceAutoSave);
               updateGUIElements(asyncTest);
            }

         }
      }

      static void doMemoryEstimate(bool forceAutoSave)
      {
         using (PerfSection p3 = new PerfSection("doMemoryEstimate"))
         {

            mMemoryUsageItems.Clear();
            estimateTerrainMemory();
            estimateSimMemory(forceAutoSave);
         }
      }
      static void updateGUIElements(bool asyncTest)
      {
         //update our main GUI progress bar (previous element?)
         float amt = giveMemoryUsagePercent();
         if (asyncTest)
         {
            ThreadSafeMessageList.MessagePacket mp = new ThreadSafeMessageList.MessagePacket();
            mp.mCallback = TerrainGlobals.getTerrainFrontEnd().setMemoryEstimateGUIValue;
            mp.mDataObject = amt;
            CoreGlobals.getEditorMain().mMessageList.enqueueMessage(mp);
         }
         else
         {
            TerrainGlobals.getTerrainFrontEnd().setMemoryEstimateGUIValue(null, amt);
         }
      }
      //---------------------------------------
      static void bw_Work(object sender, DoWorkEventArgs e)
      {
         int hackStatePacket = (int)e.Argument;
         bool forceAutoSave = ((hackStatePacket & 0x02) != 0) ? true : false;
         bool asyncTest = ((hackStatePacket & 0x01) != 0) ? true : false;

         doMemoryEstimate(forceAutoSave);
         updateGUIElements(asyncTest);
      }

   }

}