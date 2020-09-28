using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using System.Threading;

using UIHelper;
using Rendering;
using SimEditor;
using EditorCore;
using EditorCore.Statistics;
using Export360;

namespace Terrain
{
   /// <summary>
   /// Most of this is from the main.cpp
   /// 
   /// Most of it will probably change
   /// 
   /// A few small C# improvements have been made
   ///   -exposing *commands* as functions and using custom attributes to generate a ui
   ///   -exposing a few tool settings as properties (which later generate a ui)
   /// 
   /// </summary>


   public enum eMapSizes
   {
      cMap_Tiny_128v      = 128,     //
      cMap_Tiny_256v       = 256,     //

      cMap_Small_512v = 512,
      cMap_Small_640v = 640,
      cMap_Small_768v = 768,
      cMap_Small_896v = 896,

      cMap_Large_1024v = 1024,
      cMap_Large_1280v = 1280,
      cMap_Large_1536v = 1536,
      cMap_Large_1792v = 1792,
      cMap_Large_2048v = 2048,

      cMap_Sizes_Count = 11
   };

   public enum eSimQuality
   {
      cQuality_Min = 32,
      cQuality_Low = 16,
      cQuality_Med = 8,
      cQuality_High = 4,
      cQuality_Max = 2,
      cQuality_Count = 5
   }


   //CLM these params now encompass everything to create terrain
   public class TerrainCreationParams
   {
      public static float cTileSpacingOne = 1.0f;

      //simshit
      public int mNumSimXTiles = 64;
      public int mNumSimZTiles = 64;
      public float mSimTileSpacing = 1;
      public int mSimToVisMultiplier = 2;
      //visshit
      public int mNumVisXVerts = 256;
      public int mNumVisZVerts = 256;
      public float mVisTileSpacing = cTileSpacingOne;
      //corolationshit
      
      public void initFromVisData(int numXVisVerts,int numZVisVerts, float visTileScale, float numVisVertsPerSimVert)
      {
         numVisVertsPerSimVert = 4; // CLM [07.27.07] REDUCED FOR LARGER QUADNODES
            //8;//CLM [05.25.07] LOCKED to 8:1
            

         mNumVisXVerts = numXVisVerts;
         mNumVisZVerts = numZVisVerts;
         mVisTileSpacing = visTileScale;

         mNumSimXTiles = (int)(numXVisVerts / (float)numVisVertsPerSimVert);
         mNumSimZTiles = (int)(numZVisVerts / (float)numVisVertsPerSimVert);
         mSimTileSpacing = visTileScale * numVisVertsPerSimVert;
         mSimToVisMultiplier =  (int)numVisVertsPerSimVert;
      }

      public void initFromSimData(int simXTiles, int simZTiles, float simTileSize, int densityMultiplier)
      {
         float VisTileSpacing = simTileSize /(float)densityMultiplier;
         int NumVisXVerts = (int)((simXTiles + 1) * densityMultiplier);
         int NumVisZVerts = (int)((simZTiles + 1) * densityMultiplier);

         //CLM this code path is legacy.. use the new one
         initFromVisData(NumVisXVerts, NumVisZVerts, VisTileSpacing, densityMultiplier);
      }
      
   }

   [TopicType(Name = "Terrain")]
   public class BTerrainFrontend : IEditor, ITerrain
   {

      public BCameraManager mCameraManager = new BCameraManager();

      BRenderDebugAxis mRenderAxisPrim = null;
     

      //EDITOR STUFF
      float brushRad = 5;
      float brushHotspot = 0.5f;
      float brushIntensity = 1.0f;
      float brushAlpha = 1.0f;
      float brushRotation = 0;
      Terrain.BrushInfo.eIntersectionShape mIntersectionShape = Terrain.BrushInfo.eIntersectionShape.Sphere;
      bool mEdgePushMode = false;
      public bool mbPerlin = false;

      

   
      //WINDOW STUFF
 

      public BTerrainFrontend()
      {
        // setDefaultTextures();
         setDefaultMasks();
         CoreGlobals.getEditorMain().Register(this);
      }

     


      //Shared Interface
      public void setCameraPos(Vector3 pos)
      {
         mCameraManager.mEye = pos;
      }
      public void setCameraTarget(Vector3 target)
      {
         mCameraManager.mLookAt = target;
      }
      public Vector3 getCameraPos()
      {
         return mCameraManager.mEye;
      }
      public Vector3 getCameraTarget()
      {
         return mCameraManager.mLookAt;
      }

      public void snapCamera(BCameraManager.eCamSnaps snapMode)
      {
         mCameraManager.snapCamera(snapMode);
      }
      public BFrustum getFustrum()
      {
         return TerrainGlobals.getTerrain().getFrustum();

      }
      public void simEditorLightAdd(int simEditorObjectIndex)
      {
         LightManager.registerLight(simEditorObjectIndex);
      }
      public void simEditorLightDelete(int simEditorObjectIndex)
      {
         LightManager.freeLight(simEditorObjectIndex);
      }
      public void simEditorLightMoved(int simEditorObjectIndex)
      {
         LightManager.lightChangedMajor(simEditorObjectIndex);
      }
      public float getWorldSizeX()
      {
         return TerrainGlobals.getTerrain().getWorldSizeX();
      }
      public float getWorldSizeZ()
      {
         return TerrainGlobals.getTerrain().getWorldSizeZ();
      }

      public void getMaskTileBounds(ref int minX, ref int maxX, ref int minZ, ref int maxZ)
      {
         minX = Masking.mCurrSelectionMaskExtends.minX;
         maxX = Masking.mCurrSelectionMaskExtends.maxX;
         minZ = Masking.mCurrSelectionMaskExtends.minZ;
         maxZ = Masking.mCurrSelectionMaskExtends.maxZ;
      }
      public bool isVertexMasked(int x, int z)
      {
         float amt = 0;
         return Masking.isPointSelected(x, z, ref amt);
      }
      public Vector3 getTerrainPos(int x, int z)
      {
         return TerrainGlobals.getTerrain().getPostDeformPos(x, z);
      }
      public float getTerrainHeight(int x, int z)
      {
         return TerrainGlobals.getTerrain().getPostDeformPos(x, z).Y;
      }

      public IMask getMask()
      {
         return Masking.getCurrSelectionMaskWeights();
      }
      public IMask getBaseMask()
      {
         return Masking.getBaseMaskingMaskWeights();

      }
      public int getNumXVerts()
      {
         return TerrainGlobals.getTerrain().getNumXVerts();
      }
      public Vector3 getBBMin() { return TerrainGlobals.getTerrain().getBBMin(); }
      public Vector3 getBBMax() { return TerrainGlobals.getTerrain().getBBMax(); }


      #region Open, Save, New
      //Commands

      void createBlankTerrain(TerrainCreationParams param)
      {
         TerrainGlobals.getTexturing().destroyActiveDecals();
         SimTerrainType.clearActiveSet();
         SimTerrainType.loadTerrainTypes();
         UndoManager.clearAllUndos();

         CoreGlobals.getEditorMain().mIGUI.ReloadVisibleTextureThumbnails(false);

         using (PerfSection p1 = new PerfSection("destroy"))
         {
            TerrainGlobals.getTerrain().destroy();
         }
         using (PerfSection p2 = new PerfSection("createBlank"))
         {
            TerrainGlobals.getTerrain().createBlank(param);
         }
         using (PerfSection p3 = new PerfSection("createTexturesFromSimDefs"))
         {
            TerrainGlobals.getTexturing().createTexturesFromSimDefs();
            
         }
         using (PerfSection p4 = new PerfSection("reloadActiveTextures"))
         {
            TerrainGlobals.getTexturing().reloadActiveTextures(true);
            TerrainGlobals.getTexturing().reloadActiveDecals(true);
         }
      }

      public void OpenFile()
      {
         UIManager.Pause();
         OpenFileDialog d = new OpenFileDialog();
         if (d.ShowDialog() == DialogResult.OK)
         {
            LoadTed(d.FileName);
         }
         UIManager.UnPause();
      }
      public void SaveFile()
      {
         UIManager.Pause();
         SaveFileDialog d = new SaveFileDialog();
         if (d.ShowDialog() == DialogResult.OK)
         {
            SaveFile(d.FileName);
         }
         UIManager.UnPause();
      }
      
      public void NewTerrain(TerrainCreationParams param)
      {
         using (PerfSection p2 = new PerfSection("NewTerrain"))
         {
            //CoreGlobals.getEditorMain().mIGUI
            //if (MessageBox.Show("Are you sure you want to create a new map, you will loose any unsaved data.","Create new map?", MessageBoxButtons.OKCancel) == DialogResult.OK)
            {
               SimGlobals.getSimMain().ClearSim();
               using (PerfSection p3 = new PerfSection("createBlankTerrain"))
               {
                  createBlankTerrain(param);
               }
               // reset name
               CoreGlobals.ResetWorkingPaths();

               TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeNone);

               
            }
            snapCamera(BCameraManager.eCamSnaps.cCamSnap_Map_Center);
         }
      }

      public void SaveFile(string fileName)
      {
         using (PerfSection p2 = new PerfSection("Backup .TED"))
         {
            if (CoreGlobals.FatalSaveError == false)
            { 
               FileUtilities.BackupScenarioFile(fileName);               
            }
         }
         if (fileName.Contains(".TED"))
         {
            TEDIO.SaveTEDFile(fileName);
         }

     //    TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeNone);
      }


      
      private void LoadTed(string filename)
      {
         if (!File.Exists(filename))
         {
            MessageBox.Show(filename + " not found.  No terrain data loaded.");
            return;
         }

         if (filename.Contains(".TED"))
         {
            TerrainGlobals.getTerrain().destroy();
            TerrainGlobals.getEditor().destroy();
            UndoManager.clearAllUndos();

            TEDIO.LoadTEDFile(filename);


            snapCamera(BCameraManager.eCamSnaps.cCamSnap_Map_Center);
         }

      }
      private void SaveTed(string filename)
      {
         SaveFile(filename);
      }

      //private void SaveScenario(string filename, string tedname, string lightsetfile)
      //{
      //   SimGlobals.getSimMain().SaveScenario(filename, tedname, lightsetfile);

      //}
      public void LoadScenario(string filename)
      {
         SimGlobals.getSimMain().LoadScenario(filename);
         SimGlobals.getSimMain().LoadScenarioArtObjects(Path.ChangeExtension(filename, "sc2"));
         SimGlobals.getSimMain().LoadScenarioSoundObjects(Path.ChangeExtension(filename, "sc3"));

      }

      string giveLowestTEDPathInPath(string path)
      {
         //CLM This will recursivly search the path for a given .TED file
         // it will return the one closest to the origional path
         if (path == CoreGlobals.getWorkPaths().mGameScenarioDirectory)
         {
            return "";// WTF ""; //we didn't find any..
         }

         string[] files = Directory.GetFiles(path, "*.TED");
         if(files.Length==0)
         {
            //we didn't find anything here, move up a level.
            path = path.Substring(0, path.LastIndexOf(@"\"));
            return giveLowestTEDPathInPath(path);
         }

         return path;
      }
      public void LoadProject(string scenariofilename)
      {
         LoadProject(scenariofilename, false);
      }

      static string sTedname = "";
      static bool sTedLoaded = false;
      static BTerrainFrontend sThis = null;
      static void LoadTedThread()
      {
         sThis.LoadTed(sTedname);
         sTedLoaded = true;
      }
      public void LoadProject(string scenariofilename, bool ignoreSimObjLoad)
      {
         try
         {
            string directory = Path.GetDirectoryName(scenariofilename);

            bool threadedload = true;

            Thread t = null;
            if (threadedload)
            {
               PerfSection p1 = new PerfSection("KickoffTed");
               CoreGlobals.TerrainFile = Path.GetFileNameWithoutExtension(scenariofilename) + ".TED";  //hack?
               string tedPath = giveLowestTEDPathInPath(directory);
               string tedname = Path.Combine(tedPath, CoreGlobals.TerrainFile);
               CoreGlobals.TerrainDirectory = tedPath;
               //LoadTed(tedname);
               sTedname = tedname;
               sTedLoaded = false;
               sThis = this;
               t = new Thread(new ThreadStart(LoadTedThread), 0);
               t.Start();
               t.Name = "LoadTEd";
               p1.Complete();
            }

            //System.Threading.Thread.CurrentThread.Name = "MainThread";

            using (PerfSection p2 = new PerfSection("Sim.LoadScenario"))
            {
               CoreGlobals.ScenarioFile = scenariofilename;
               CoreGlobals.ScenarioDirectory = directory;
               if (CoreGlobals.IsBatchExport)
               {
                  SimGlobals.getSimMain().LoadScenario(scenariofilename, ignoreSimObjLoad, true, false);
               }
               else
               {
                  SimGlobals.getSimMain().LoadScenario(scenariofilename);
                  SimGlobals.getSimMain().LoadScenarioArtObjects(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioArtFile));
                  SimGlobals.getSimMain().LoadScenarioSoundObjects(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioSoundFile));

               }

               //CLM load TED specified in the scenario file
            }
            if (threadedload)
            {
               using (PerfSection p3 = new PerfSection("WaitTed"))
               {
                  while (t.IsAlive && sTedLoaded == false)
                  {
                     System.Threading.Thread.Sleep(1000);
                  }
               }
            }
            if (!threadedload)
            {
               string tedPath = giveLowestTEDPathInPath(directory);
               string tedname = Path.Combine(tedPath, CoreGlobals.TerrainFile);
               CoreGlobals.TerrainDirectory = tedPath;
               LoadTed(tedname);
            }



            if(CoreGlobals.mPlayableBoundsMinX == -1) CoreGlobals.mPlayableBoundsMinX =0;
            if(CoreGlobals.mPlayableBoundsMinZ == -1) CoreGlobals.mPlayableBoundsMinZ =0;
            if(CoreGlobals.mPlayableBoundsMaxX == -1) CoreGlobals.mPlayableBoundsMaxX =TerrainGlobals.getTerrain().getNumXVerts();
            if(CoreGlobals.mPlayableBoundsMaxZ == -1) CoreGlobals.mPlayableBoundsMaxZ = TerrainGlobals.getTerrain().getNumXVerts();


         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }
      //public void SaveProject(string scenariofilename)
      //{
      //   SaveProject(scenariofilename, true);
      //}

      //public bool IsAltScenario(string scenariofilename)
      //{
      //   string directory = Path.GetDirectoryName(scenariofilename);
      //   string terrainFile = Path.ChangeExtension(Path.GetFileName(scenariofilename), "TED");
      //   string terrainPath = giveLowestTEDPathInPath(directory);

      //   if (terrainPath == "")//we didn't find a ted above us, so we must be a unique export
      //   {
      //      terrainPath = directory;
      //   }
      //   else//We found another .TED Above us, ensure it's not one already in our root
      //   {
      //      string[] files = Directory.GetFiles(terrainPath, "*.TED");
      //      if (files.Length != 0)
      //         terrainFile = Path.GetFileName(files[0]);
      //   }

      //   bool isAltScenario = (directory != terrainPath);
      //   return isAltScenario;
      //}


      public void SaveProject(string scenariofilename)
      //public void SaveProject(string scenariofilename, bool updateWorkingDirectories, bool saveTerrain)
      {
         try
         {
            string directory = Path.GetDirectoryName(scenariofilename);
            string terrainFile = Path.ChangeExtension(Path.GetFileName(scenariofilename), "TED");
            string terrainPath = directory;
            string tedname = Path.Combine(terrainPath, terrainFile);

            #region oldbob
            //Logic for alternate "bob" scenarios.  This was disabled due to bob cancelation, but could be re-enabled later. 
            /*
            string terrainPath = giveLowestTEDPathInPath(directory);
           
            if(terrainPath == "")//we didn't find a ted above us, so we must be a unique export
            {
               terrainPath = directory;
            }
            else//We found another .TED Above us, ensure it's not one already in our root
            {
               string[] files = Directory.GetFiles(terrainPath, "*.TED");
               if (files.Length != 0)
                  terrainFile = Path.GetFileName(files[0]);
            }
            bool isAltScenario = (directory != terrainPath);
            //derive our full TED name for export
            string tedname = Path.Combine(terrainPath, terrainFile);//CoreGlobals.TerrainFile);

            if (!updateWorkingDirectories)//CLM assume we're doing a temp export.
            {
               tedname = Path.Combine(directory, Path.GetFileNameWithoutExtension(scenariofilename) + ".TED");
               isAltScenario = false;
            }
            if (tedname == "")
            {
               tedname = Path.ChangeExtension(scenariofilename, ".TED");
            }            
            
            //CLM don't save the terrain if we're saving an alternate scenario
            if (!isAltScenario)
            {
               string fullTedName = tedname;
               SaveTed(tedname);
            }
            */
            #endregion

            SaveTed(tedname);


            //APF we used to save the scenario here, but that has been extracted
            //SaveScenario(scenariofilename, tedname, lightsetfile);

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }


      }

      #endregion


      #region brushButtons
      [ToolCommand(Name = "Height Only Brush", Icon = "normalbase2")]
      public void BrushesHeightbrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertHeightEdit);
         TerrainGlobals.getEditor().newHeightBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Height Only Brush");         

      }

      [ToolCommand(Name = "Normal Brush", Icon = "normalbase")]
      public void BrushesStdBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertStd);
         TerrainGlobals.getEditor().newStdBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Normal Brush");         

      }
      [ToolCommand(Name = "Layer Brush", Icon = "layer")]
      public void BrushesLayerBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertLayer);
         TerrainGlobals.getEditor().newLayerBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Layer Brush");         

      }
      [ToolCommand(Name = "Set Height Brush", Icon = "set")]
      public void BrushesSetHeightBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertSetHeight);
         TerrainGlobals.getEditor().newSetHeightBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Set Height Brush");         

      }


    

      [ToolCommand(Name = "Inflate Brush", Icon = "inflate")]
      public void BrushesInflatBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertInflat);
         TerrainGlobals.getEditor().newInflateBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Inflate Brush");         

      }


      [ToolCommand(Name = "Pinch Brush", Icon = "pinch")]
      public void BrushesPinchBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertPinch);
         TerrainGlobals.getEditor().newPinchBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Pinch Brush");         

      }
      [ToolCommand(Name = "Smooth Brush", Icon = "SmoothBrush")]
      public void BrushesSmoothBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertSmooth);
         TerrainGlobals.getEditor().newSmoothBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Smooth Brush");         

      }

      [ToolCommand(Name = "Smudge Brush", Icon = "smudge")]
      public void BrushesSmudgeBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertAvgEdit);
         TerrainGlobals.getEditor().newAvgHeightBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Smudge Brush");         

      }
      

      [ToolCommand(Name = "Push Pull Brush", Icon = "PushBrush")]
      public void BrushesPushBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertPushEdit);
         TerrainGlobals.getEditor().newPushBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Push Pull Brush");         
      }



      [ToolCommand(Name = "Uniform Brush", Icon = "Uniform")]
      public void Uniform()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertUniform);
         TerrainGlobals.getEditor().newUniformBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Uniform Brush");         
      }
      [ToolCommand(Name = "Scalar Brush", Icon = "scaleBrush")]
      public void BrushesScaleBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertScale);
         TerrainGlobals.getEditor().newScaleBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Scalar Brush");

      }
      [ToolCommand(Name = "Skirt Height Brush", Icon = "Skirt")]
      public void SkirtHeight()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertSkirtHeight);
         TerrainGlobals.getEditor().newSkirtHeightBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(20.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Skirt Brush");
      }

      [ToolCommand(Name = "Vertex Alpha Brush", Icon = "vertAlpha")]
      public void BrushesAlphaBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertAlpha);
         TerrainGlobals.getEditor().newAlphaBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Alpha Brush");
      }

       [ToolCommand(Name = "Tessellation Override Brush", Icon = "vertTess")]
      public void BrushesTessellationBrush()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertTessellation);
         TerrainGlobals.getEditor().newTesselationBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Tessellation Brush");
      }


      [ToolCommand(Name = "Vertex Translation Widget", Icon = "transWidget")]
      public void VertTransWidget()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertWidgetTrans);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Vert Trans Widget");
         TerrainGlobals.getEditor().updateWidgetPos();
      }

      [ToolCommand(Name = "Shape Select Mode", Icon = "shapeSelect")]
      public void shapeSelect()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeShapeSelect);
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().cleanBrush();
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Shape Select Mode");
      }

      [ToolCommand(Name = "Custom Shape Select Mode", Icon = "customSelect")]
      public void customShapeSelect()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeCustomShape);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Custom Shape Select Mode");
      }
       
      [ToolCommand(Name = "Hydrology", Icon = "water")]
      public void waterPanelShow()
      {
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().mIGUI.ShowDialog("WaterPanel");
      }

      [ToolCommand(Name = "Texturing", Icon = "texturing")]
      public void texturingPanelShow()
      {
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().mIGUI.ShowDialog("TexturingPanel");
      }

      [ToolCommand(Name = "Roads", Icon = "road")]
      public void roadPanelShow()
      {
         CoreGlobals.getEditorMain().mIGUI.ShowDialog("RoadPanel");
      }
      [ToolCommand(Name = "Foliage", Icon = "foliage")]
      public void foliagePanelShow()
      {
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().mIGUI.ShowDialog("FoliagePanel");
      }

      [ToolCommand(Name = "Numeric Terrain")]
      public void numericTerrain()
      {
         System.Windows.Forms.Button b = new System.Windows.Forms.Button();
         System.Windows.Forms.Button b2 = new System.Windows.Forms.Button();
         b.Text = "Apply";
         b.Click += new EventHandler(b_Click);
         b2.Text = "-Apply";
         b2.Click += new EventHandler(b2_Click);
         BetterPropertyGrid g = new BetterPropertyGrid();
         g.SelectedObject = mNumericInput;

         PopupEditor pe = new PopupEditor();
         Panel p = new Panel();
         p.Controls.Add(b);
         p.Controls.Add(b2);
         p.Controls.Add(g);
         g.Top = b.Bottom;
         b2.Left = b.Right + 10;

         pe.ShowPopup((Control)CoreGlobals.getEditorMain().mIGUI, p);
      }
      void b_Click(object sender, EventArgs e)
      {         
         TerrainGlobals.getEditor().translateVerts2(mNumericInput.mVector);

         CoreGlobals.getEditorMain().mOneFrame = true;

      }
      void b2_Click(object sender, EventArgs e)
      {
         Vector3 invert = new Vector3(-mNumericInput.mVector.X, -mNumericInput.mVector.Y, -mNumericInput.mVector.Z);
         TerrainGlobals.getEditor().translateVerts2(invert);

         CoreGlobals.getEditorMain().mOneFrame = true;
      }
      VectorInput mNumericInput = new VectorInput();
      class VectorInput
      {
         public Vector3 mVector = new Vector3(0, 0, 0);
         public float X
         {
            get
            {
               return mVector.X;
            }
            set
            {
               mVector.X = value;
            }
         }
         public float Y
         {
            get
            {
               return mVector.Y;
            }
            set
            {
               mVector.Y = value;
            }
         }
         public float Z
         {
            get
            {
               return mVector.Z;
            }
            set
            {
               mVector.Z = value;
            }
         }
      }


     

      //[ToolCommand(Name = "Normal Dot", Icon = "normalbase")]
      public void StdDot()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertStdDot);
         TerrainGlobals.getEditor().newStdBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);

      }

      //[ToolCommand(Name = "Inflate Dot", Icon = "inflate")]
      public void InflatDot()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeVertInflatDot);
         TerrainGlobals.getEditor().newInflateBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }
      //[ToolCommand(Name = "Toggle Noise", Icon = "noise", ButtonType = eControlType.cCheck)]
      public void ToggleNoise()
      {
         mbPerlin = !mbPerlin;
      }

      public bool NoiseEnabled
      {
         get
         {
            return mbPerlin;
         }
         set
         {
            mbPerlin = value;
         }
      }

      public void Undo()
      {
         TerrainGlobals.getEditor().UndoChanges();
      }

      public void Redo()
      {
         TerrainGlobals.getEditor().RedoChanges();
      }
      public void Copy()
      {
         TerrainGlobals.getEditor().CopySelected(false);
      }

      public void Cut(bool upperCut)
      {
         TerrainGlobals.getEditor().CutSelected(upperCut);


      }

      [ToolCommand(Name = "Clipart")]
      public void PasteMode()
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModePasteMode);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         TerrainGlobals.getEditor().newPushBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(brushRad);

         CoreGlobals.getEditorMain().mIGUI.ShowDialog("ClipArtPicker");
         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Paste Terrain");         

      }
      public bool mbPasteMode = true;




      public void ToggleViewSim()
      {
         TerrainGlobals.getEditor().toggleViewSim();
      }


      public void UpdateSimRep()
      {
         TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(),TerrainGlobals.getTerrain().getTileScale(),true);
      }

      public void ToggleEdgeMove()
      {
         mEdgePushMode = !mEdgePushMode;
         if (mEdgePushMode)
            mCameraManager.setCamMode(BCameraManager.eCamModes.cCamMode_RTS);
         else
            mCameraManager.setCamMode(BCameraManager.eCamModes.cCamMode_Free);
      }

      public void ToggleEChunkBounds()
      {
         TerrainGlobals.getEditor().setRenderNodeBounds(!TerrainGlobals.getEditor().getRenderNodeBounds());
      }

      #endregion

      #region sim rep buttons
      [ToolCommand(Name = "SimRepVisible", Icon = "simrep")]
      public void simRepVisible()
      {
         if (!TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
         TerrainGlobals.getEditor().toggleViewSim();
      }
      [ToolCommand(Name = "Update Sim Rep", Icon = "simrepUpdate")]
      public void updateSimRep()
      {
         TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
         if (!TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights)
            TerrainGlobals.getEditor().toggleViewSim();

      }

      [ToolCommand(Name = "Sim Passibility", Icon = "simpassible")]
      public void simPassibilityMode()
      {
         TerrainGlobals.getEditor().newSimPassibilityBrush();

        // if (!TerrainGlobals.getEditor().mRenderSimHeights)
            TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
            TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSimPassibility);
         
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Passable Brush");
      }
      [ToolCommand(Name = "Sim Buildable", Icon = "simBuildible")]
      public void simBuildableMode()
      {
         TerrainGlobals.getEditor().newSimBuildabilityBrush();

        // if (!TerrainGlobals.getEditor().mRenderSimHeights)
            TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
            TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSimBuildibility);
         
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Buildable Brush");
      }
      [ToolCommand(Name = "Sim Flood Passable", Icon = "simFloodPassable")]
      public void simFloodPassableMode()
      {
         TerrainGlobals.getEditor().newSimFloodPassableBrush();

         // if (!TerrainGlobals.getEditor().mRenderSimHeights)
         TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
         TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSimFloodPassibility);

         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("FloodPassable Brush");
      }
      [ToolCommand(Name = "Sim Scarab Passable", Icon = "simScarabPassable")]
      public void simScarabPassableMode()
      {
         TerrainGlobals.getEditor().newSimScarabPassableBrush();

         // if (!TerrainGlobals.getEditor().mRenderSimHeights)
         TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
         TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSimScarabPassibility);

         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("ScarabPassable Brush");
      }

      [ToolCommand(Name = "Sim Tile Type", Icon = "simTileType")]
      public void simTileTypeMode()
      {
         TerrainGlobals.getEditor().newSimTileTypeBrush();

         // if (!TerrainGlobals.getEditor().mRenderSimHeights)
         TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
         TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSimTileType);

         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("TileType Brush");
      }

      [ToolCommand(Name = "Sim Heights Override", Icon = "simHeightBrush")]
      public void simHeightMode()
      {
         if (!TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
         TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSimHeightEdit);
         TerrainGlobals.getEditor().newSimHeightBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Sim Heights Override");   

         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }
      [ToolCommand(Name = "Sim Set Height Override", Icon = "simSetHeight")]
      public void simSetHeightMode()
      {
         if (!TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
         TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSimSetHeight);
         TerrainGlobals.getEditor().newSimSetHeightBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }
      [ToolCommand(Name = "Sim Smooth Height Override", Icon = "SimSmoothheights")]
      public void simSmoothMode()
      {
         if (!TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
         TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSimSmoothHeight);
         TerrainGlobals.getEditor().newSimSmoothBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }
      [ToolCommand(Name = "Sim Erase Height Override", Icon = "simEraseHeight")]
      public void simEraseHeightMode()
      {
         if (!TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().update();//.updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
         TerrainGlobals.getEditor().getSimRep().getHeightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeSimEraseHeight);
         TerrainGlobals.getEditor().newSimEraseHeightBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }

      [ToolCommand(Name = "Clear ALL Height Override", Icon = "clearsimrep")]
      public void simClearHeightOverride()
      {
         if (MessageBox.Show("Clear ALL sim height override values?", "You sure?", MessageBoxButtons.YesNo) != DialogResult.Yes)
            return;

         TerrainGlobals.getEditor().getSimRep().getHeightRep().clearHeightOverride();
         updateSimRep();
      }
#endregion

      [ToolCommand(Name = "|" )]
      public void bracer0()
      {

      }

      #region camera mode buttons
      [ToolCommand(Name = "Camera Heights Visible", Icon = "cameraRep")]
      public void cameraHeightsVisibleToggle()
      {

         if (!TerrainGlobals.getEditor().getCameraRep().mRenderHeights)
            TerrainGlobals.getEditor().getCameraRep().recalculateHeights();
         TerrainGlobals.getEditor().getCameraRep().mRenderHeights = !TerrainGlobals.getEditor().getCameraRep().mRenderHeights;
      }

      [ToolCommand(Name = "Camera Heights Update", Icon = "CameraUpdateRep")]
      public void cameraHeightsUpdate()
      {
         TerrainGlobals.getEditor().getCameraRep().recalculateHeights();
         if (!TerrainGlobals.getEditor().getCameraRep().mRenderHeights)
            TerrainGlobals.getEditor().getCameraRep().mRenderHeights = !TerrainGlobals.getEditor().getCameraRep().mRenderHeights;
      }

      [ToolCommand(Name = "Camera Heights Override", Icon = "CameraHeightRep")]
      public void cameraHeightMode()
      {
         if (!TerrainGlobals.getEditor().getCameraRep().mRenderHeights)
            TerrainGlobals.getEditor().getCameraRep().recalculateHeights();
         TerrainGlobals.getEditor().getCameraRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeCameraHeight);
         TerrainGlobals.getEditor().newCameraHeightsBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Camera Heights Override");

         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }

      [ToolCommand(Name = "Camera Set Heights Override", Icon = "CameraSetRep")]
      public void cameraSetHeightMode()
      {
         if (!TerrainGlobals.getEditor().getCameraRep().mRenderHeights)
            TerrainGlobals.getEditor().getCameraRep().recalculateHeights();
         TerrainGlobals.getEditor().getCameraRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeCameraSetHeight);
         TerrainGlobals.getEditor().newCameraSetHeightsBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Camera Set Heights Override");

         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }

      [ToolCommand(Name = "Camera Smooth Heights Override", Icon = "CameraSmoothRep")]
      public void cameraSmoothMode()
      {
         if (!TerrainGlobals.getEditor().getCameraRep().mRenderHeights)
            TerrainGlobals.getEditor().getCameraRep().recalculateHeights();
         TerrainGlobals.getEditor().getCameraRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeCameraHeightSmooth);
         TerrainGlobals.getEditor().newCameraSmoothBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Camera Smooth Heights Override");

         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }

      [ToolCommand(Name = "Camera Erase Height Override", Icon = "CameraEraseRep")]
      public void cameraEraseHeightMode()
      {
         if (!TerrainGlobals.getEditor().getCameraRep().mRenderHeights)
            TerrainGlobals.getEditor().getCameraRep().recalculateHeights();
         TerrainGlobals.getEditor().getCameraRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeCameraEraseHeights);
         TerrainGlobals.getEditor().newCameraEraseBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Camera Erase Heights Override");

         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }

      [ToolCommand(Name = "Clear ALL Camera Height Overrides", Icon = "CameraEraseAllRep")]
      public void cameraClearHeightOverride()
      {
         if (MessageBox.Show("Clear ALL camera height override values?", "You sure?", MessageBoxButtons.YesNo) != DialogResult.Yes)
            return;

         TerrainGlobals.getEditor().getCameraRep().clearHeightOverride();
         TerrainGlobals.getEditor().getCameraRep().recalculateHeights();
      }
      #endregion

      #region flightrep mode buttons
      [ToolCommand(Name = "Flight Heights Visible", Icon = "FlightRep")]
      public void flightHeightsVisibleToggle()
      {

         if (!TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().getFlightRep().recalculateHeights();
         TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights = !TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights;
      }

      [ToolCommand(Name = "Flight Heights Update", Icon = "FlightUpdateRep")]
      public void flightHeightsUpdate()
      {
         TerrainGlobals.getEditor().getSimRep().getFlightRep().recalculateHeights();
         if (!TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights = !TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights;
      }

      [ToolCommand(Name = "Flight Heights Override", Icon = "FlightHeightRep")]
      public void flightHeightMode()
      {
         if (!TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().getFlightRep().recalculateHeights();
         TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeFlightHeight);
         TerrainGlobals.getEditor().newFlightHeightsBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Flight Heights Override");

         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }

      [ToolCommand(Name = "Flight Set Heights Override", Icon = "FlightSetRep")]
      public void flightSetHeightMode()
      {
         if (!TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().getFlightRep().recalculateHeights();
         TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeFlightSetHeight);
         TerrainGlobals.getEditor().newFlightSetHeightsBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Flight Set Heights Override");

         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }

      [ToolCommand(Name = "Flight Smooth Heights Override", Icon = "FlightSmoothRep")]
      public void flightSmoothMode()
      {
         if (!TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().getFlightRep().recalculateHeights();
         TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeFlightHeightSmooth);
         TerrainGlobals.getEditor().newFlightSmoothBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Flight Smooth Heights Override");

         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }

      [ToolCommand(Name = "Flight Erase Height Override", Icon = "FlightEraseRep")]
      public void flightEraseHeightMode()
      {
         if (!TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights)
            TerrainGlobals.getEditor().getSimRep().getFlightRep().recalculateHeights();
         TerrainGlobals.getEditor().getSimRep().getFlightRep().mRenderHeights = true;

         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeFlightEraseHeights);
         TerrainGlobals.getEditor().newFlightEraseBrush();
         TerrainGlobals.getEditor().setVertBrushRadiusFactor(1.0f);
         TerrainGlobals.getEditor().setVertBrushRadius(10);//brushRad);
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Flight Erase Heights Override");

         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;
      }

      [ToolCommand(Name = "Clear ALL Flight Height Overrides", Icon = "FlightEraseAllRep")]
      public void flightClearHeightOverride()
      {
         if (MessageBox.Show("Clear ALL flight height override values?", "You sure?", MessageBoxButtons.YesNo) != DialogResult.Yes)
            return;

         TerrainGlobals.getEditor().getSimRep().getFlightRep().clearHeightOverride();
         TerrainGlobals.getEditor().getSimRep().getFlightRep().recalculateHeights();
      }
      #endregion

      //[ToolCommand(Name = "RealLOS.Toggle")]
      //public void realLOSVisibleToggle()
      //{
      //   if (!TerrainGlobals.getEditor().getRealLOSRep().mRenderHeights)
      //      TerrainGlobals.getEditor().getRealLOSRep().recalculateHeights();
      //   TerrainGlobals.getEditor().getRealLOSRep().mRenderHeights = !TerrainGlobals.getEditor().getRealLOSRep().mRenderHeights;
      //}
      //[ToolCommand(Name = "RealLOS.Calculate")]
      //public void realLOSCalculate()
      //{
      //   if (!TerrainGlobals.getEditor().getRealLOSRep().mRenderHeights)
      //      TerrainGlobals.getEditor().getRealLOSRep().recalculateHeights();
      //   TerrainGlobals.getEditor().getRealLOSRep().CalculateLos();
      //}
      //[ToolCommand(Name = "RealLOS.Test")]
      //public void realLOSTest()
      //{
      //   if (!TerrainGlobals.getEditor().getRealLOSRep().mRenderHeights)
      //      TerrainGlobals.getEditor().getRealLOSRep().recalculateHeights();
      //   TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeRealLOSUnit);
      //}

      


      #region Texture UI interface

      List<Texture> mMaskList = new List<Texture>();
      string[] mMaskTexturefilenames = null;
      int mSelectedMaskIndex = 0;
      void setDefaultMasks()
      {
         mMaskTexturefilenames = Directory.GetFiles(CoreGlobals.getWorkPaths().mBrushMasks, "*.bmp");
      }
      void createMasks()
      {
         foreach (string file in mMaskTexturefilenames)
            LoadMaskTexture(file);
      }
      public string[] getMaskTextures()
      {
         return mMaskTexturefilenames;
      }
      public Texture getSelectedMaskTexture()
      {
         return mMaskList[mSelectedMaskIndex];
      }
      public void LoadMaskTexture(string filename)
      {
        mMaskList.Add(TextureLoader.FromFile(BRenderDevice.getDevice(), filename));
         
         if (TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeTexEdit)
         {
            mSelectedMaskIndex = mMaskList.Count - 1;
            TerrainGlobals.getEditor().newSplatBrush(SelectedTextureIndex, mMaskList[mSelectedMaskIndex], mMaskTexturefilenames[mSelectedMaskIndex], mCurrentBrushStroke);
         }
      }

      #endregion
      //--------------------------------------
      public void update()
      {
         try
         {
            TerrainGlobals.getEditor().update(0);
            SimGlobals.getSimMain().update();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());
         }
      }

      public void render()
      {
         //if (mbDeviceLost || mbMinimized)
         //   return;

         //if (BRenderDevice.IsDeviceLost())
         //   return;
         try
         {
            mCameraManager.camUpdate();

            BRenderDevice.clear(true, true, unchecked((int)0x8C003F3F),1.0f,0);
            
            BRenderDevice.beginScene();

            BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.None);

            if (TerrainGlobals.getEditor().isShowSelectionCameraEnabled())
            {
               TerrainGlobals.getEditor().renderSelectionThumbnail(false,true,true);
               BRenderDevice.getDevice().Clear(ClearFlags.ZBuffer | ClearFlags.Target, unchecked((int)0x8C003F3F), 1.0f, 0);
            }

            TerrainGlobals.getEditor().render();

            SimGlobals.getSimMain().render(TerrainGlobals.getEditor().isRenderFogEnabled());

            TerrainGlobals.getEditor().renderSimReps();

            TerrainGlobals.getEditor().renderWidget();

            renderAxis();

            BRenderDevice.endScene();
            if (BRenderDevice.IsDeviceLost() == false )
            {

               BRenderDevice.present();
            }

         }
         catch(Microsoft.DirectX.Direct3D.DeviceLostException devEx)
         {
            //TerrainGlobals.getTexturing().d3dDeviceLost();
            throw devEx;
            
         }
         catch (System.Exception ex)
         {
            ex.ToString();
         }
      }
      public void renderAxis()
      {
         if (mRenderAxisPrim == null) return;
         Matrix w = BRenderDevice.getDevice().Transform.World;
         Matrix p = BRenderDevice.getDevice().Transform.Projection;
         Viewport vpp = BRenderDevice.getDevice().Viewport;

         Viewport vp = new Viewport();
         vp.X = 0;
         vp.Y = vpp.Height - (int)(vpp.Height * 0.1f);
         vp.Width = (int)(vpp.Width * 0.1f);
         vp.Height = (int)(vpp.Height * 0.1f);
         BRenderDevice.getDevice().Viewport = vp;
         
         
         Vector3 tdir = mCameraManager.mEye - mCameraManager.mLookAt;
         tdir = BMathLib.Normalize(tdir);

         BRenderDevice.getDevice().Transform.World = Matrix.Translation(mCameraManager.mEye - (tdir *3.0f));
         BRenderDevice.getDevice().Transform.Projection = Matrix.PerspectiveFovLH(Geometry.DegreeToRadian(45), (float)BRenderDevice.getWidth() / (float)BRenderDevice.getHeight(), 0.5f, 6.0f);
         

         mRenderAxisPrim.render();

         BRenderDevice.getDevice().Transform.World=w;
         BRenderDevice.getDevice().Transform.Projection = p;
         BRenderDevice.getDevice().Viewport = vpp;
      }
      public Image renderThumbnail(bool doTexturing, bool doObjects)
      {
         try
         {
            BRenderDevice.getDevice().Clear(ClearFlags.ZBuffer | ClearFlags.Target, unchecked((int)0x8C003F3F), 1.0f, 0);
            BRenderDevice.getDevice().BeginScene();
            BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.None);
            TerrainGlobals.getEditor().renderSelectionThumbnail(false, doTexturing, doObjects);
            BRenderDevice.getDevice().Clear(ClearFlags.ZBuffer | ClearFlags.Target, unchecked((int)0x8C003F3F), 1.0f, 0);
            BRenderDevice.getDevice().EndScene();
            return TerrainGlobals.getEditor().GetThumbnailBitmap();
         }
         catch (System.Exception ex)
         {
            ex.ToString();
        //    mbDeviceLost = true;
         }
         return null;
      }

      void RenderModeInput()
      {
         //If you want one of these settings add it to the combobox.
         if (UIManager.GetAsyncKeyStateB(Key.D1))
         {
            TerrainGlobals.getEditor().setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderFull);
            TerrainGlobals.getTexturing().reloadCachedVisuals();
         }
         if (UIManager.GetAsyncKeyStateB(Key.D2)) TerrainGlobals.getEditor().setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderFullWireframe);
         if (UIManager.GetAsyncKeyStateB(Key.D3)) TerrainGlobals.getEditor().setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderFlat);
         if (UIManager.GetAsyncKeyStateB(Key.D4)) TerrainGlobals.getEditor().setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderFlatWireframe);
         if (UIManager.GetAsyncKeyStateB(Key.D5)) TerrainGlobals.getEditor().setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderFullOverlay);
         if (UIManager.GetAsyncKeyStateB(Key.D6)) TerrainGlobals.getEditor().setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderAmbientOcclusion);
         if (UIManager.GetAsyncKeyStateB(Key.D7))
         {
            TerrainGlobals.getEditor().setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderTextureSelectRender);
            TerrainGlobals.getTexturing().reloadCachedVisuals();
         }
         if (UIManager.GetAsyncKeyStateB(Key.D8)) TerrainGlobals.getEditor().setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderTextureNormals);
         if (UIManager.GetAsyncKeyStateB(Key.D9)) TerrainGlobals.getEditor().setRenderMode(BTerrainEditor.eEditorRenderMode.cTerrainPerfEval);
         
      }
      public void setRenderMode(BTerrainEditor.eEditorRenderMode mode)
      {
         TerrainGlobals.getEditor().setRenderMode(mode);
      }


      public void setMemoryEstimateString(string str, object obj)
      {
         string msg = (string)obj;
         if (CoreGlobals.getEditorMain().mIGUI != null)
            CoreGlobals.getEditorMain().mIGUI.setMemoryEstimateString(msg);
      }
      public void updateMemoryEstimate(bool forceAutoSave,bool asyncTest)
      {
         ExportMemoryEstimate.estimateMemory(forceAutoSave,asyncTest);
      }
      public void setMemoryEstimateGUIValue(string msg, object obj)
      {
         float amt = (float)obj;
         if (CoreGlobals.getEditorMain().mIGUI != null)
          CoreGlobals.getEditorMain().mIGUI.setTerrainMemoryPercent(amt);
      }



      public void input()
      {
         try
         {

            if (CoreGlobals.getEditorMain().mIGUI != null)
               CoreGlobals.getEditorMain().mIGUI.clearCursorLocationLabel();

            SimGlobals.getSimMain().input();

            MouseWheelInput();

            mCameraManager.CameraMovement();

            TexturingInput();

            BrushInput();

            RenderModeInput();

           

            if (UIManager.GetAsyncKeyStateB(Key.LeftBracket))
               this.VertBrushRadius-= 0.1f;
            if (UIManager.GetAsyncKeyStateB(Key.RightBracket))
               this.VertBrushRadius+= 0.1f;

            if (UIManager.GetAsyncKeyStateB(Key.Minus))
               this.VertBrushRadius -= 1.0f;
            if (UIManager.GetAsyncKeyStateB(Key.Equals))
               this.VertBrushRadius += 1.0f;

           

            //PASS INPUT THROUGH TO EDITOR
            TerrainGlobals.getEditor().input();

            UIManager.WheelDelta = 0;
         }  
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());

         }  
      }

      public void init()
      {
         mRenderAxisPrim = new BRenderDebugAxis(1);

         //load our terrain textures
         TerrainGlobals.getTexturing().reloadActiveTextures(true);
         TerrainGlobals.getTexturing().reloadActiveDecals(true);
         createMasks();

         SimGlobals.getSimMain().init();
         SimGlobals.getSimMain().ClearSim();

         Matrix View;
         View = Matrix.LookAtLH(mCameraManager.mEye, mCameraManager.mLookAt, mCameraManager.mUp);
         BRenderDevice.getDevice().SetTransform(TransformType.View, View);

      }
      public void destroy()
      {
         TerrainGlobals.getTerrain().destroy();
         SimGlobals.getSimMain().deinit();
         SimGlobals.getSimMain().ClearSim();
      }

      public void deinitDeviceData()
      {
         //RENDERING
         if (TerrainGlobals.getTerrain() != null && TerrainGlobals.getTerrain().getQuadNodeRoot() != null)
         {
            TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
            TerrainGlobals.getTerrain().getQuadNodeRoot().freeCacheHandles();
         }

         TerrainGlobals.getTexturing().d3dDeviceLost();
         TerrainGlobals.getVisual().d3dDeviceLost();

         //reset sim
     //    SimGlobals.getSimMain().ClearVisuals();

         
      }
      public void initDeviceData()
      {
         //WE MAY NOT HAVE TERRAIN INITALIZED YET!
         if (TerrainGlobals.getTerrain().getQuadNodeRoot() != null)
         {
           // TerrainGlobals.getTerrain().postCreate();

            TerrainGlobals.getTexturing().reloadActiveTextures(true);
            TerrainGlobals.getTexturing().reloadActiveDecals(true);

            //reset editor
            TerrainGlobals.getEditor().deviceReset();

            TerrainGlobals.getTexturing().d3dDeviceRestored();
            TerrainGlobals.getVisual().d3dDeviceRestored();

         }
      }

      

      
      bool mbMinimized = false;
      public void Minimize()
      {
         if (mbMinimized == false)
         {
            mbMinimized = true;
         }
      }
      public void UnMinimize()
      {
         if (mbMinimized == true)
         {
            mbMinimized = false;
         }
      }


      
      const float cMinVertBrushIntensity = 0.0f;
      const float cMaxVertBrushIntensity = 1.0f;

      const float cMinVertBrushRadius = 0.0f;
      const float cMaxVertBrushRadius = 40.0f;
      
      const float cMinVertBrushHotSpot = 0.0f;
      const float cMaxVertBrushHotspot = 1.0f;
      

      //These properties seem to be a pretty good way to deal with tool settings
      public float VertBushIntensity
      {
         set
         {
            if (value != brushIntensity && value >= cMinVertBrushIntensity && value <= cMaxVertBrushIntensity)
            {
               brushIntensity = value;
               TerrainGlobals.getEditor().setVertBrushIntensity(brushIntensity);
            }
            TerrainGlobals.getEditor().SetPasteDirty();
         }
         get
         {
            return brushIntensity;
         }
      }
      
      public float VertBrushRadius
      {
         set
         {
            if (value != brushRad && value > cMinVertBrushRadius && value < cMaxVertBrushRadius)
            {
               brushRad = value;
               TerrainGlobals.getEditor().setVertBrushRadius(brushRad);
            }
            TerrainGlobals.getEditor().SetPasteDirty();
         }
         get
         {
            return brushRad;
         }
      }

      public float TexBrushRotation
      {
         set
         {
            if (value != brushRotation)
            {
               brushRotation = value;
               TerrainGlobals.getEditor().setTextureBrushRotation(brushRotation);
            }
            TerrainGlobals.getEditor().SetPasteDirty();
         }
         get
         {
            return brushRotation;
         }
      }

      public float VertBrushHotspot
      {
         set
         {
            if (value != brushHotspot && value >= cMinVertBrushHotSpot && value <= cMaxVertBrushHotspot)
            {
               brushHotspot = value;
               TerrainGlobals.getEditor().setVertBrushHotspot(brushHotspot);
            }
         }
         get
         {
            return brushHotspot;
         }
      }


      public Terrain.BrushInfo.eIntersectionShape VertBrushIntersectionShape
      {
         set
         {
            mIntersectionShape = value;
            TerrainGlobals.getEditor().setVertBrushIntersectionShape(mIntersectionShape);
         }
         get
         {
            return mIntersectionShape;
         }
      }


      BTerrainEditor.eTessOverrideVal mTesselationValue = BTerrainEditor.eTessOverrideVal.cTess_None;
      public BTerrainEditor.eTessOverrideVal TesselationValue
      {
         set
         {
            mTesselationValue = value;
            TerrainGlobals.getEditor().setTessellationBrushValue(mTesselationValue);
         }
         get 
         {
            return mTesselationValue;
         }
      }

      bool mScalarBrushXZOnly = false;
      public bool ScalarBrushXZOnly
      {
         set
         {
            mScalarBrushXZOnly = value;
            TerrainGlobals.getEditor().setScaleBrushBrushMode(mScalarBrushXZOnly);
         }
         get
         {
            return mScalarBrushXZOnly;
         }
      }

      bool mSmudgeBrushUseNormal = false;
      public bool SmudgeBrushUseNormal
      {
         set
         {
            mSmudgeBrushUseNormal = value;
            TerrainGlobals.getEditor().setSmudgeBrushBrushMode(mSmudgeBrushUseNormal);
         }
         get
         {
            return mSmudgeBrushUseNormal;
         }
      }

      


      public enum ePasteOperation
      {
         Add,
         AddYOnly,
         Replace,
         Merge
      }
      ePasteOperation mPasteOperation = ePasteOperation.Add;
      public ePasteOperation PasteOperation
      {
         set
         {
            mPasteOperation = value;

            TerrainGlobals.getEditor().setPasteOperation( value );

            TerrainGlobals.getEditor().SetPasteDirty();
         }
         get
         {
            return mPasteOperation;
         }
      }

      public void OKPaste()
      {
         TerrainGlobals.getEditor().OKPaste();
      }
      public void CancelPaste()
      {
         TerrainGlobals.getEditor().CancelPaste();

      }


      void MouseWheelInput()
      {

         float WHEEL_DELTA = 50;

         int zDelta = UIManager.WheelDelta;

         if (zDelta == 0)
            return;
         if (SimGlobals.getSimMain().isUsingDesignerControls() && (UIManager.GetAsyncKeyStateB(Key.LeftAlt) || UIManager.GetAsyncKeyStateB(Key.RightAlt)))
            return;

         {
            if (UIManager.GetAsyncKeyStateB(Key.LeftShift))
               TexBrushRotation += ((float)zDelta / WHEEL_DELTA) * 0.05f;
            else
               VertBrushRadius += ((float)zDelta / WHEEL_DELTA) * 0.25f;
            CoreGlobals.getEditorMain().mPhoenixScenarioEditor.UpdateSliders();
         }
      }

      BrushStroke mCurrentBrushStroke = new BrushStroke();
      public BrushStroke GetMainBrushStroke()
      {
         return mCurrentBrushStroke;
      }

      public Dictionary<string, Modifier> mModifiers = new Dictionary<string, Modifier>();
      public void RegisterModifier(Modifier mod)
      {
         if (!mModifiers.ContainsKey(mod.mName))
            mModifiers.Add(mod.mName, mod);
      }




      int mSelectedTextureIndex = 0;
      public int SelectedTextureIndex
      {
         set
         {
            if ((TerrainGlobals.getEditor().getMode() != BTerrainEditor.eEditorMode.cModeTexEdit)
            || ((value != mSelectedTextureIndex) && value >= 0 && value < SimTerrainType.mActiveWorkingSet.Count))
            {               
               mSelectedTextureIndex = value;
               TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeTexEdit);
               TerrainGlobals.getEditor().newSplatBrush(mSelectedTextureIndex, mMaskList[mSelectedMaskIndex], mMaskTexturefilenames[mSelectedMaskIndex], mCurrentBrushStroke);

               CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

               CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Texture Paint");         

            }
         }
         get
         {
            if ((mSelectedTextureIndex < 0 || mSelectedTextureIndex >= SimTerrainType.mActiveWorkingSet.Count))
               mSelectedTextureIndex = 0;

            return mSelectedTextureIndex;
         }
      }



      int mSelectedDecalIndex = 0;
      public int SelectedDecalIndex
      {
         set
         {
            if ((TerrainGlobals.getEditor().getMode() != BTerrainEditor.eEditorMode.cModeDecalEdit)
            || ((value != mSelectedDecalIndex) && value >= 0 && value < TerrainGlobals.getTexturing().getActiveDecalCount()))
            {
               mSelectedDecalIndex = value;

               string fname = TerrainGlobals.getTexturing().getActiveDecal(mSelectedDecalIndex).mFilename;
               string ext = Path.GetExtension(fname);
               string tName = fname.Substring(0, fname.LastIndexOf("_df")) + "_op" + ext;

               TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeDecalEdit);
               TerrainGlobals.getEditor().newDecalBrush(mSelectedDecalIndex, mCurrentBrushStroke, tName);

               CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

               CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Texture Paint");
            }
         }
         get
         {
            return mSelectedDecalIndex;
         }


      }

      public int SelectedMaskIndex
      {
         set
         {
            if ((TerrainGlobals.getEditor().getMode() != BTerrainEditor.eEditorMode.cModeTexEdit)
            || ((value < mMaskList.Count) && (value >= 0)) )
            {
               mSelectedMaskIndex = value;
               TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeTexEdit);
               TerrainGlobals.getEditor().newSplatBrush(SelectedTextureIndex, mMaskList[mSelectedMaskIndex], mMaskTexturefilenames[mSelectedMaskIndex], mCurrentBrushStroke);

               CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cTerrain;

               CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Texture Paint");         

            }

         }
         get
         {
            return mSelectedMaskIndex;
         }
      }

      public bool mbZoomedOutMode = false;
      int mNumTerrainHandles = 27;
      public int NumTerrainHandles
      {
         set
         {
            if (value > 27)
               mbZoomedOutMode = true;
            else
               mbZoomedOutMode = false;

            mNumTerrainHandles = value;
            TerrainGlobals.getTerrain().recreate();
         }
         get
         {
            return mNumTerrainHandles;
         }


      }


      void TexturingInput()
      {
         if (UIManager.GetAsyncKeyStateB(Key.LeftControl) || UIManager.GetAsyncKeyStateB(Key.RightControl))
            return;

      }
      //-----------------------------------------------------------------------------
      void BrushInput()
      {


         if (UIManager.GetAsyncKeyStateB(Key.Grave))
         {
            if (!keyDownLastFrame[41])
            {
               Masking.invertSelectionMask();

               keyDownLastFrame[41] = true;
            }
         }
         else
            keyDownLastFrame[41] = false;
      }
      bool[] keyDownLastFrame = new bool[256];

      OldNoiseFunction mNoiseFunction = new BasicNoise();

      public OldNoiseFunction getNoiseFunction() { return mNoiseFunction; }

      public bool intersectPointOnTerrain(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt)
      {
         BTerrainQuadNode node = null;
         return TerrainGlobals.getTerrain().rayIntersects(ref origin, ref dir, ref intersectionPt, ref node, true);
      }

      public bool intersectPointOnTerrain(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal, ref int X, ref int Z)
      {
         BTerrainQuadNode node = null;

         return TerrainGlobals.getTerrain().rayIntersects(ref origin, ref dir, ref intersectionPt, ref intersectionNormal, ref X, ref Z, ref node, true);
         
     
      }
      public bool intersectPointOnTerrainSimRep(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt)
      {
         float3 ret = new float3();
         bool ok=TerrainGlobals.getEditor().getSimRep().getHeightRep().getClosestIntersectionPoint(new float3(origin), new float3(dir), out ret);
         intersectionPt = ret.toVec3();
         return ok;
      }

      public float getSimRepHeightInterpolated(ref Vector3 origin)
      {
         return TerrainGlobals.getEditor().getSimRep().getHeightRep().getInterpolatedHeightParametric(origin.X,origin.Z);
      }
      

      public void exportRefinedRQTTerrainToObj(string targetFilename,bool onlySelectedVerts)
      {
         int mWidth = TerrainGlobals.getTerrain().getNumXVerts();
         int mHeight = TerrainGlobals.getTerrain().getNumZVerts();

         
         //refine the terrain
         int numXChunks = (int)(mWidth / (float)BTerrainQuadNode.getMaxNodeDepth());
         int nodeWidth = (int)BTerrainQuadNode.getMaxNodeWidth();

         Export360.ExportSettings es = TerrainGlobals.getTerrain().getExportSettings();
         float eLOD = 0.25f;// es.RefineEpsilon;// es.RefineEpsilon * 4; //Expand this to ensure it's lower res.
         Export360.XTD_Refine refiner = null;
         refiner = new Export360.XTD_Refine();
         refiner.refineTerrain(eLOD, nodeWidth + 1);

         //for each chunk, generate the IB, walk all the polies of the IB
         OBJFile output = new OBJFile();
         output.addObject("Terrain");

         int []inds=null;
         int intCount = 0;
         int primCount = 0;

         int index,x,z=0;
         float refAmt = 0;
         float tmp = 0;
         int[] xInds = new int[3];
         int[] zInds = new int[3];
         Vector3[] verts = new Vector3[3];
         Vector3[] nrms = new Vector3[3];
 
         for(int xc =0; xc<numXChunks; xc++)
         {
            for(int zc =0; zc<numXChunks; zc++)
            {
               int xVertOffset = xc * nodeWidth;
               int zVertOffset = zc * nodeWidth;
               
               refiner.genChunkInds(xVertOffset, zVertOffset, ref inds, ref intCount, ref primCount);
               for(int i=0;i<primCount;i++)
               {
                  int numImpassible = 0;
                  int numNotMasked = 0;
                  //generate the XZ from the primitive information
                  bool checkOK = false;
                  for (int k = 0; k < 3; k++)
                  {
                     index = inds[i * 3 + k];
                     xInds[k] = xVertOffset + (index / (nodeWidth + 1));
                     zInds[k] = zVertOffset + (index % (nodeWidth + 1));

                     //early out tests
                     if (onlySelectedVerts)
                     {
                        if (Masking.isPointSelected(zInds[k], xInds[k], ref refAmt))
                        {
                           checkOK = true;
                           numNotMasked++;
                        }
                     }

                     verts[k] = TerrainGlobals.getTerrain().getPostDeformPos(xInds[k], zInds[k]);
                     nrms[k] = TerrainGlobals.getTerrain().getPostDeformNormal(xInds[k], zInds[k]);
                   
                  }


                  if (checkOK && (numNotMasked >= 2 || numImpassible >= 2))
                     continue;


                  //ensure winding.
                  //make sure our normal is pointing upwards
                  Vector3 aV = Vector3.Normalize(verts[2] - verts[0]);
                  Vector3 bV = Vector3.Normalize(verts[1] - verts[0]);
                  Vector3 nrml = BMathLib.Cross(aV, bV);
                  if (BMathLib.Dot(ref nrml, ref BMathLib.unitY) < 0)
                  {
                     Vector3 a = verts[0];
                     verts[0] = verts[2];
                     verts[2] = a;

                     a = nrms[0];
                     nrms[0] = nrms[2];
                     nrms[2] = a;
                  }

                  output.addTriangle(0, verts[0], nrms[0], verts[1], nrms[1], verts[2], nrms[2]);

               }
            }
         }

         if (File.Exists(targetFilename))
            File.Delete(targetFilename);
         output.write(new FileStream(targetFilename, FileMode.OpenOrCreate));
         output = null;
         
      }

      void pruneTriListForMasked(List<Export360.XTD_DeulanyTIN.triangle> triList, List<Point> inputVerts)
      {
         //remove any trs that span impassible terrain
         for (int i = 0; i < triList.Count; i++)
         {
            Export360.XTD_DeulanyTIN.triangle tri = triList[i];


            //find the centroid of this triangle. bail if it's surrounded by impassible area
            Vector3 p0v = new Vector3(inputVerts[tri.vertIndex[0]].X , 0, inputVerts[tri.vertIndex[0]].Y );
            Vector3 p1v = new Vector3(inputVerts[tri.vertIndex[1]].X , 0, inputVerts[tri.vertIndex[1]].Y );
            Vector3 p2v = new Vector3(inputVerts[tri.vertIndex[2]].X , 0, inputVerts[tri.vertIndex[2]].Y );

            Vector3 centroid = BMathLib.centroidOfTriangle(ref p0v, ref p1v, ref p2v);

            Point cx = new Point((int)centroid.X, (int)centroid.Z);
            float amt = 0;
            if (!Masking.isPointSelected(cx.X, cx.Y, ref amt))
            {
               triList.RemoveAt(i);
               i--;
               continue;
            }

            triList[i].myIndex = i;
         }
      }
      
      public void exportRefinedTINTerrainToObj(string targetFilename, bool onlySelectedVerts, float eLOD)
      {
         Export360.ExportSettings es = TerrainGlobals.getTerrain().getExportSettings();
         int mWidth = TerrainGlobals.getTerrain().getNumXVerts();
         int mHeight = TerrainGlobals.getTerrain().getNumZVerts();


       //  float eLOD = 0.25f;// es.RefineEpsilon;
         Refinement.ErrorMetricRefine refiner = new Refinement.ErrorMetricRefine();
         refiner.init(mWidth, mHeight);
         refiner.refine(eLOD);

         List<Point> PrimeVerts = new List<Point>();


         Export360.XTD_DeulanyTIN tin = new Export360.XTD_DeulanyTIN();

         for (int x = 0; x < mWidth; x++)
         {
            for (int z = 0; z < mHeight; z++)
            {

               if (refiner.getMarkedPt(x, z))
               {
                  if (onlySelectedVerts)
                  {
                     float amt = 0;
                     if (!Masking.isPointSelected(x, z, ref amt))
                        continue;
                  }

                  PrimeVerts.Add(new Point(x, z));

               }
            }
         }

         float magicScale = 64;

         //create our TIN
         for (int i = 0; i < PrimeVerts.Count; i++)
         {
            Vector3 vertexPos = TerrainGlobals.getTerrain().getPostDeformPos(PrimeVerts[i].X, PrimeVerts[i].Y);

            vertexPos.X *= -magicScale;
            vertexPos.Y *= magicScale;
            vertexPos.Z *= -magicScale;

            tin.addVertex(vertexPos);
         }

         tin.Triangulate();


         //ouput our data to the file.
         OBJFile output = new OBJFile();
         output.addObject("Terrain");
         List<int> inds = new List<int>();
         List<Export360.XTD_DeulanyTIN.triangle> triList = tin.getTriList();

         if (onlySelectedVerts)
            pruneTriListForMasked(triList, PrimeVerts);


         for (int i = 0; i < triList.Count; i++)
         {
            Export360.XTD_DeulanyTIN.triangle tri = triList[i];

            //if this tri has 0 area, get rid of it..


            //CLM deulany outputs a different clockwise winding than i want..
            //this SHOULD be a proper clockwise winding test
            inds.Add(tri.vertIndex[0]);
            inds.Add(tri.vertIndex[2]);
            inds.Add(tri.vertIndex[1]);

         }

         output.addVertexList(0, tin.getVertList(), inds);


         // Now also include all objects that are currently selected.
         //
         foreach (EditorObject edObj in SimGlobals.getSimMain().mSelectedEditorObjects)
         {
            SimObject simObj = edObj as SimObject;
            if (simObj != null)
            {
               if(simObj.mVisual != null)
               {
                  // Add object
                  int objectId = output.addObject(simObj.Name);

                  simObj.mVisual.burnIntoObjFile(output, objectId, simObj.getMatrix());
               }
            }
         }



         if (File.Exists(targetFilename))
            File.Delete(targetFilename);
         output.write(new FileStream(targetFilename, FileMode.OpenOrCreate));
         output = null;

      }

      public enum eMirrorTerrainType
      {
         eMirror_nXpX = 0,
         eMirror_pXnX,

         eMirror_nYpY,
         eMirror_pYnY,
      }
      public void mirrorTerrain(eMirrorTerrainType type)
      {
         if(MessageBox.Show("WARNING! This will replace part of the terrain with the mirrored version! Is this what you want?","WARNING!", MessageBoxButtons.YesNo) != DialogResult.Yes)
            return;


         //flip horizontal
         int workWidth = TerrainGlobals.getTerrain().getNumXVerts() - 1;
         int pivot = TerrainGlobals.getTerrain().getNumXVerts() >> 1;

         int numXNodes = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
         int nodeworkWidth = numXNodes - 1;
         int nodePivot = numXNodes>>1;

         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();
         BTerrainQuadNode[] leafNodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
         
         switch(type)
         {
            case eMirrorTerrainType.eMirror_pYnY:

               for (int x = 0; x < TerrainGlobals.getTerrain().getNumXVerts(); x++)
               {
                  for (int z = 0; z < pivot; z++)
                  {
                     int dstIndex = x * TerrainGlobals.getTerrain().getNumXVerts() + z;

                     int srcIndex = x * TerrainGlobals.getTerrain().getNumXVerts() + (workWidth - z);

                     detail[dstIndex] = detail[srcIndex];
                  }
               }
               //exchange texture handles
               for (int x = 0; x < numXNodes; x++)
               {
                  for (int z = 0; z < nodePivot; z++)
                  {
                     int dstIndex = x + numXNodes * z;

                     int srcIndex = x + numXNodes * (nodeworkWidth - z);

                     leafNodes[srcIndex].mLayerContainer.copyTo(ref leafNodes[dstIndex].mLayerContainer);
                     leafNodes[dstIndex].mLayerContainer.flip(BTerrainLayerContainer.eFlipType.eFlip_Vert);
                  }
               }
               break;
            case eMirrorTerrainType.eMirror_nYpY:

               for (int x = 0; x < TerrainGlobals.getTerrain().getNumXVerts(); x++)
               {
                  for (int z = 0; z < pivot; z++)
                  {
                     int srcIndex = x * TerrainGlobals.getTerrain().getNumXVerts() + z;

                     int dstIndex = x * TerrainGlobals.getTerrain().getNumXVerts() + (workWidth - z);

                     detail[dstIndex] = detail[srcIndex];
                  }
               }
               //exchange texture handles
               for (int x = 0; x < numXNodes; x++)
               {
                  for (int z = 0; z < nodePivot; z++)
                  {
                     int srcIndex = x + numXNodes * z;

                     int dstIndex = x + numXNodes * (nodeworkWidth - z);

                     leafNodes[srcIndex].mLayerContainer.copyTo(ref leafNodes[dstIndex].mLayerContainer);
                     leafNodes[dstIndex].mLayerContainer.flip(BTerrainLayerContainer.eFlipType.eFlip_Vert);
                  }
               }
               break;


            case eMirrorTerrainType.eMirror_nXpX:
               for (int x = 0; x < pivot; x++)
               {
                  for (int z = 0; z < TerrainGlobals.getTerrain().getNumZVerts(); z++)
                  {
                     int srcIndex = x * TerrainGlobals.getTerrain().getNumXVerts() + z;

                     int dstIndex = (workWidth - x) * TerrainGlobals.getTerrain().getNumXVerts() + z;

                     detail[dstIndex] = detail[srcIndex];
                  }
               }
               //exchange texture handles
               for (int x = 0; x < nodePivot; x++)
               {
                  for (int z = 0; z < numXNodes; z++)
                  {
                     int srcIndex = x + numXNodes * z;

                     int dstIndex = (nodeworkWidth - x) + numXNodes * z;

                     leafNodes[srcIndex].mLayerContainer.copyTo(ref leafNodes[dstIndex].mLayerContainer);
                     leafNodes[dstIndex].mLayerContainer.flip(BTerrainLayerContainer.eFlipType.eFlip_Horiz);
                  }
               }
               break;
            case eMirrorTerrainType.eMirror_pXnX:
            default:
               for (int x = 0; x < pivot; x++)
               {
                  for (int z = 0; z < TerrainGlobals.getTerrain().getNumZVerts(); z++)
                  {
                     int dstIndex = x * TerrainGlobals.getTerrain().getNumXVerts() + z;

                     int srcIndex = (workWidth - x) * TerrainGlobals.getTerrain().getNumXVerts() + z;

                     detail[dstIndex] = detail[srcIndex];
                  }
               }

               //exchange texture handles
               for (int x = 0; x < nodePivot; x++)
               {
                  for (int z = 0; z < numXNodes; z++)
                  {
                     int dstIndex = x + numXNodes * z;

                     int srcIndex = (nodeworkWidth - x) + numXNodes * z;

                     leafNodes[srcIndex].mLayerContainer.copyTo(ref leafNodes[dstIndex].mLayerContainer);
                     leafNodes[dstIndex].mLayerContainer.flip(BTerrainLayerContainer.eFlipType.eFlip_Horiz);
                  }
               }

               break;
         };
         


         
         for (uint i = 0; i < leafNodes.Length; i++)
            leafNodes[i].mDirty = true;

         BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(),
                              TerrainGlobals.getEditor().getNormals(),
                              TerrainGlobals.getTerrain().getTileScale(), 
                              TerrainGlobals.getTerrain().getNumXVerts(),

                              0, 
                              TerrainGlobals.getTerrain().getNumXVerts(), 
                              0, 
                              TerrainGlobals.getTerrain().getNumZVerts());

         TerrainGlobals.getTerrain().refreshTerrainVisuals();
         TerrainGlobals.getTerrain().getQuadNodeRoot().rebuildDirty(BRenderDevice.getDevice());
         
      }
   }
 


}
