using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Microsoft.DirectX;
using Xceed.DockingWindows;

using Terrain;
using UIHelper;
using SimEditor;
using System.IO;
using EditorCore;
using Rendering;
using VisualEditor;
using GameDatabase;
using Export360;

using PhoenixEditor.ClientTabPages;
using PhoenixEditor.ScenarioEditor;

using ParticleSystem;
using ModelSystem;

namespace PhoenixEditor
{
   public partial class MainWindow : Form, IMainWindow
   {
      static public MainWindow mMainWindow = null;
      static private FileSystemWatcher s_fileWatcher = null;

      #region Main window Forms and DirectX init code
      public MainWindow()
      {
         this.WindowState = FormWindowState.Maximized;
         InitializeComponent();
         this.Refresh();
         try
         {
            
            CoreGlobals.loadSettingsFile(Path.Combine(CoreGlobals.getWorkPaths().mEditorSettings, "usersettings.xml" ) );
            CoreGlobals.getSettingsFile().Save();

            BRenderDevice.createDevice(this, this.Width, this.Height, false);

            MainWindowInit();

            CoreGlobals.UsingPerforce = CoreGlobals.getPerforce().getConnection().Setup();


            BRenderDevice.mDeviceLost += new EventHandler(BRenderDevice_mDeviceLost);
            BRenderDevice.mDeviceReset += new EventHandler(BRenderDevice_mDeviceReset);
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

         
      }

      void BRenderDevice_mDeviceReset(object sender, EventArgs e)
      {
         UIManager.InitKeyBoardCapture();
         this.deviceResored();
      }

      void BRenderDevice_mDeviceLost(object sender, EventArgs e)
      {
         this.deviceLost();
      }

      bool m_bQuickStartVisualEditor = false;
      bool m_bQuickStartParticleEditor = false;
      bool m_bQuickStartScenarioEditor = false;

      List<string> m_filesToLoad = new List<string>();

      List<string> m_filesToReload = new List<string>();
      bool m_bFilesChanged = false;

      public MainWindow(string[] args)
      {
         InitializeComponent();
         this.WindowState = FormWindowState.Maximized;

         try
         {
            BRenderDevice.createDevice(this, this.Width, this.Height, false);

            MainWindowInit();

            // Process command line arguments
            //

            foreach (string argument in args)
            {
               // Check if argument starts with a -
               if(argument.StartsWith("-"))
               {
                  // get following character
                  char optionChar = argument[1];

                  switch(optionChar)
                  {
                     case 'V':
                     case 'v':
                        m_bQuickStartVisualEditor = true;
                        break;

                     case 'P':
                     case 'p':
                        m_bQuickStartParticleEditor = true;
                        break;

                     case 'S':
                     case 's':
                        m_bQuickStartScenarioEditor = true;
                        break;
                  }
               }
               else
               {
                  // files to load
                  if (File.Exists(argument))
                  {
                     m_filesToLoad.Add(argument);
                  }
               }
            }

            CoreGlobals.UsingPerforce = CoreGlobals.getPerforce().getConnection().Setup();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      /// <summary>
      /// This is for batch the exporter only...
      /// </summary>
      /// <param name="filename"></param>
      public MainWindow(string filename)
      {
         InitializeComponent();
         return;  
#if false
         try
         {
            timer1.Enabled = true;
            MainWindowInit();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

         LoadProject(filename);
         string xtdName = Path.Combine(CoreGlobals.ProjectDirectory, Path.GetFileNameWithoutExtension(filename) + ".XTD");
         ExportXTD(xtdName,true);

         Application.Exit();

         throw new System.Exception("finished");
#endif
         //this
      }


      public bool mD3DSnapToClient = true;

      public void setD3DSnapToClient(bool snap)
      {
         mD3DSnapToClient = snap;
      }

      public void MainWindowInit()
      {
         mMainWindow = this;
         EditorCore.CoreGlobals.getEditorMain().Register(this);

         // File Watcher
         if (s_fileWatcher == null)
         {
            s_fileWatcher = new FileSystemWatcher();
            s_fileWatcher.Path = EditorCore.CoreGlobals.getWorkPaths().mGameArtDirectory;
            s_fileWatcher.Filter = "*.vis";
            //s_fileWatcher.NotifyFilter = NotifyFilters.LastWrite;
            s_fileWatcher.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.CreationTime | NotifyFilters.Size;

            s_fileWatcher.Changed += new FileSystemEventHandler(FileWatcher_Changed);
            s_fileWatcher.IncludeSubdirectories = true;
            s_fileWatcher.EnableRaisingEvents = true;
         }
      }

      bool mbMainInit = false; //only do this init once...
      public void Init(Control window)
      {
         using (PerfSection p3 = new PerfSection("PhoenixEditor.Init()"))
         {
            if (mbMainInit == true)
               return;
            mbMainInit = true;
            mDockLayoutManager.AllowDocking = true;


            UIManager.InitUIManager(this, window);
            UIManager.InitKeyBoardCapture();

            //CLM - THE FOLLOWING SHOULD BE REFACTORED TO SCENARIO EDITOR!

            //RenderModeToolStripComboBox.SelectedIndex = 0;

            mDockLayoutManager.SuspendLayout();
            this.ShowDialog("WorldObjectList");
            this.ShowDialog("UnitPicker");
            using (PerfSection p4 = new PerfSection("TexturingPanel()"))
            {
               this.ShowDialog("TexturingPanel");
            }
            this.ShowDialog("MaskLayers");
            this.ShowDialog("BrushSettings");
            this.ShowDialog("UnitPicker");
            mDockLayoutManager.ResumeLayout();

            using (PerfSection p4 = new PerfSection("mUnitPicker.DelayedLoad()"))
            {
               mUnitPicker.DelayedLoad();
            }
         }
      }

      Form mSplash = new Form();
      public void ShowSplash()
      {
         mSplash.BackgroundImage = Image.FromFile("loading.JPG");
         mSplash.Height = mSplash.BackgroundImage.Height;
         mSplash.Width = mSplash.BackgroundImage.Width;
         mSplash.Show();
      }
      public void CloseSplash()
      {
         if (mSplash != null)
         {
            mSplash.Close();
            mSplash = null;
         }
      }
      public Point getWindowLocation()
      {
         return this.Location;
      }


      #endregion 

      #region DirectX Device Management Handlers
      public void deviceResize(int width, int height,bool force)
      {
         if (mD3DSnapToClient || force)
         {
            //issue the command to all tabs to de-init their device specific data
            MainWindow.mMainWindow.deviceLost();
            //issue the command to destroy the device
            if (BRenderDevice.resizeBackBuffer(width, height))
            {

               //issue the command to all tabs to init their device specific data
               MainWindow.mMainWindow.deviceResored();
            }
            else
            {
               
               deviceLost();
            }
         }
      }
      public void deviceLost()
      {
         for(int i=0;i<ClientTabControl.TabCount;i++)
         {
            ClientTabPage ctp = ClientTabControl.TabPages[i] as ClientTabPage;
            if(ctp!=null)
               ctp.deinitDeviceData();
         }
      }
      public void deviceResored()
      {
         for (int i = 0; i < ClientTabControl.TabCount; i++)
         {
            ClientTabPage ctp = ClientTabControl.TabPages[i] as ClientTabPage;
            if (ctp != null)
               ctp.initDeviceData();
         }
      }
      #endregion 

      #region Client Tab Pages
      //Client Tab page
      List<ClientTabPage> mParticleEditors = new List<ClientTabPage>();
      List<ClientTabPage> mVisualEditors = new List<ClientTabPage>();
   //   ClientTabPage mOnlyScenarioTab = null;
   //   ScenarioEditorPage mScenarioEditorControl = null;

      ClientTabPage mOnlyTerrainTypeTab = null;
      TerrainTypePage mTerrainTypeEditorControl = null;

      ClientTabPage mActiveTabPage = null;
      public void InitClientTabs()
      {
         //initalize the timer
         timer1.Enabled = true;
         soundTimer.Enabled = true;
      }

      //SCENERIO TAB
      void initScenarioTab(ref ScenarioEditorPage mScenarioEditorControl, ref ClientTabPage mOnlyScenarioTab)
      {
         using (PerfSection p = new PerfSection("initScenarioTab()"))
         {
            mScenarioEditorControl = new ScenarioEditorPage();
            mScenarioEditorControl.PreventDeviceResize = true;



            mOnlyScenarioTab = new ClientTabPage();
            mOnlyScenarioTab.Controls.Add(mScenarioEditorControl);

            mScenarioEditorControl.Dock = DockStyle.Fill;
            mScenarioEditorControl.init();

            mActiveTabPage = mOnlyScenarioTab;
         }
        
      }
      void giveEditorPage(ref ScenarioEditorPage mScenarioEditorControl, ref ClientTabPage mOnlyScenarioTab)
      {
         for (int i = 0; i < ClientTabControl.TabPages.Count; i++)
         {
            if (ClientTabControl.TabPages[i].Controls[0] is ScenarioEditorPage)
            {
               mOnlyScenarioTab = (ClientTabPage)(ClientTabControl.TabPages[i]);
               mScenarioEditorControl = (ScenarioEditorPage)ClientTabControl.TabPages[i].Controls[0];
               return;
            }
         }
      }

      public bool containsScenarioTab()
      {
         for (int i = 0; i < ClientTabControl.TabPages.Count; i++)
            if (ClientTabControl.TabPages[i].Controls[0] is ScenarioEditorPage)
               return true;
         return false;
      }

      bool mbLoaded = false;
      public bool IsSafeToUnloadScenario()
      {
         return !containsScenarioTab();
      }
      public void NewScenario(TerrainCreationParams param)
      {
         try
         {

            using (PerfSection p = new PerfSection("NewScenario"))
            {
               if (IsSafeToUnloadScenario() || (MessageBox.Show("Are you sure you want to create a new map, you will loose any unsaved data.", "Create new map?", MessageBoxButtons.OKCancel) == DialogResult.OK))
               {
                  using (PerfSection p1 = new PerfSection("resizeBackBuffer()"))
                  {
                     if (mbLoaded == false)
                     {
                        BRenderDevice.resizeBackBuffer(this.Width, this.Height);

                     }
                  }

                  mbLoaded = true;

                  ClientTabPage mOnlyScenarioTab = null;
                  ScenarioEditorPage mScenarioEditorControl = null;

                  if (!containsScenarioTab())
                  {
                     initScenarioTab(ref mScenarioEditorControl, ref mOnlyScenarioTab);
                  }
                  else
                  {
                     giveEditorPage(ref mScenarioEditorControl, ref mOnlyScenarioTab);
                  }
                  

                  using (PerfSection p2 = new PerfSection("othacrap()"))
                  {
                     using (PerfSection p3 = new PerfSection("SetupTabPages()"))
                     {
                        mScenarioEditorControl.PreventDeviceResize = true;

                        //TabPage scenarioTab = new TabPage("new scenario");            
                        mOnlyScenarioTab.Text = "new scenario";

                        if (ClientTabControl.TabPages.Contains(mOnlyScenarioTab) == false)
                           ClientTabControl.TabPages.Add(mOnlyScenarioTab);   //tries to resize
                     }
                        
                     Init(mScenarioEditorControl.GetUITarget());


                     mScenarioEditorControl.NewProject(param);
                     

                     using (PerfSection p4 = new PerfSection("othacrapB()"))
                     {
                        ClientTabControl.SelectedTab = mOnlyScenarioTab;

                        SetClientFocus();

                        mScenarioEditorControl.PreventDeviceResize = false;
                     }
                  }

                  
               }
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }


      public void OpenScenario(string path)
      {
         try
         {
            using (PerfSection p = new PerfSection("OpenScenario  " + Path.GetFileName(path)))
            {
               if (IsSafeToUnloadScenario() || (MessageBox.Show("Are you sure you want to open a different map, you will loose any unsaved data.", "Create new map?", MessageBoxButtons.OKCancel) == DialogResult.OK))
               {
                  ClientTabPage mOnlyScenarioTab = null;
                  ScenarioEditorPage mScenarioEditorControl = null;

                  using (PerfSection p2 = new PerfSection("PreLoadSteps"))
                  {
                     mbLoaded = true;

 
                     if (!containsScenarioTab())
                     {
                        initScenarioTab(ref mScenarioEditorControl, ref mOnlyScenarioTab);
                     }
                     else
                     {
                        giveEditorPage(ref mScenarioEditorControl, ref mOnlyScenarioTab);
                     }

                     //if (mScenarioEditorControl.CheckPerforceLoad(path) == false)
                     //{
                     //   return;
                     //}                   

                     //else if (MessageBox.Show("Only one map can be open at a time.  Unsaved changes will be lost", "Warning", MessageBoxButtons.OKCancel) != DialogResult.OK)
                     //{
                     //   return;
                     //}

                     mOnlyScenarioTab.Text = Path.GetFileNameWithoutExtension(path);
                     if (ClientTabControl.TabPages.Contains(mOnlyScenarioTab) == false)
                        ClientTabControl.TabPages.Add(mOnlyScenarioTab);

                     Init(mScenarioEditorControl.GetUITarget());
                  }

                  mActiveTabPage.open_file(path);

                  using (PerfSection p3 = new PerfSection("PostLoadSteps"))
                  {
                     mScenarioEditorControl.PreventDeviceResize = false;
                     ClientTabControl.SelectedTab = mOnlyScenarioTab;
                     SetClientFocus();
                     mTexturingPanel.postTEDLoad(false);
                  }

               }
            }
            
            TerrainGlobals.getTerrainFrontEnd().updateMemoryEstimate(false, true);


            PerfHarness.Harness.PerfLogAddNote(TextureStats.Log());
            //System.Threading.Thread.Sleep(1000);
            //PerfHarness.Harness.PerfLogAddNote(TextureStats.Log());

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      
      void loadTEDIntoSceneCrop(string scenariofilename,ScenarioEditorPage mScenarioEditorControl)
      {
         string directory = Path.GetDirectoryName(scenariofilename);
         string tedname = Path.Combine(directory, Path.GetFileNameWithoutExtension(scenariofilename) + ".TED");


         TerrainCreationParams param = new TerrainCreationParams();
         TEDIO.TEDLoadedData dat = new TEDIO.TEDLoadedData();
         TEDIO.mShowAlerts = true;
         TEDIO.LoadTEDvLatest(tedname, ref dat, ref param);
         

         int newWidth = TerrainGlobals.getTerrain().getNumXVerts();
         int newHeight = TerrainGlobals.getTerrain().getNumZVerts();
         int oldWidth = param.mNumVisXVerts;
         int oldHeight = param.mNumVisZVerts;
         int numBorderVerts = (newWidth - oldWidth) >> 1;

         if(newWidth <=oldWidth || newHeight <=oldHeight)
         {
            MessageBox.Show("New sizes are equal to or smaller than original map. This action is not yet supported");
            return;
         }
         //createAllFromTed(dat, param);
         TEDIO.createTexturingFromTED(dat, numBorderVerts, numBorderVerts);
         TEDIO.createQNTexuringFromTED(dat, numBorderVerts, numBorderVerts);
         mTexturingPanel.postTEDLoad(false);

         //COPY VERTEX POINTS
         Vector3[] verts = TerrainGlobals.getEditor().getDetailPoints();
         int destOffsetShift = numBorderVerts * newWidth;
         for (int x = 0; x < oldWidth; x++)
         {
            for (int z = 0; z < oldHeight; z++)
            {
               int srcIndex = x * oldWidth + z;
               int DstIndex = (x + numBorderVerts) * newWidth + (z + numBorderVerts);
               verts[DstIndex] = dat.mPositions[srcIndex];
            }
         }

         //update our terrain visuals
         
         BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                   TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                   0, TerrainGlobals.getTerrain().getNumXVerts(), 0, TerrainGlobals.getTerrain().getNumXVerts());
         TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, 0, TerrainGlobals.getTerrain().getNumXVerts(),
                                                                                     0, TerrainGlobals.getTerrain().getNumXVerts());
         for (int i = 0; i < nodes.Count; i++)
            nodes[i].mDirty = true;
         
         TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());


         //OBJECT DATA!
         TerrainGlobals.getTerrainFrontEnd().LoadScenario(scenariofilename);
         mScenarioEditorControl.LoadProjectSimData(scenariofilename);

         //translate our objects
         SimGlobals.getSimMain().selectAll();
         float objectXOffset = numBorderVerts * TerrainGlobals.getTerrain().getTileScale();
         float objectZOffset = numBorderVerts * TerrainGlobals.getTerrain().getTileScale();
         SimGlobals.getSimMain().forcedObjectTranslation(objectXOffset, 0, objectZOffset);
         SimGlobals.getSimMain().unselectAll();

         //reset playable bounds since we've resized
         CoreGlobals.mPlayableBoundsMinX = 0;
         CoreGlobals.mPlayableBoundsMinZ = 0;
         CoreGlobals.mPlayableBoundsMaxX = TerrainGlobals.getTerrain().getNumXVerts();
         CoreGlobals.mPlayableBoundsMaxZ = TerrainGlobals.getTerrain().getNumZVerts();
      }
      public void OpenScenarioAsCrop(string path, TerrainCreationParams newSizeParams)
      {
         try
         {
            using (PerfSection p = new PerfSection("NewScenario"))
            {
               if (IsSafeToUnloadScenario() || (MessageBox.Show("Are you sure you want to create a new map, you will loose any unsaved data.", "Create new map?", MessageBoxButtons.OKCancel) == DialogResult.OK))
               {
                  using (PerfSection p1 = new PerfSection("resizeBackBuffer()"))
                  {
                     if (mbLoaded == false)
                     {
                        BRenderDevice.resizeBackBuffer(this.Width, this.Height);

                     }
                  }

                  mbLoaded = true;

                  ClientTabPage mOnlyScenarioTab = null;
                  ScenarioEditorPage mScenarioEditorControl = null;

                  if (!containsScenarioTab())
                  {
                     initScenarioTab(ref mScenarioEditorControl, ref mOnlyScenarioTab);
                  }
                  else
                  {
                     giveEditorPage(ref mScenarioEditorControl, ref mOnlyScenarioTab);
                  }


                  using (PerfSection p2 = new PerfSection("othacrap()"))
                  {
                     mScenarioEditorControl.PreventDeviceResize = true;

                     //TabPage scenarioTab = new TabPage("new scenario");            
                     mOnlyScenarioTab.Text = "new scenario";

                     if (ClientTabControl.TabPages.Contains(mOnlyScenarioTab) == false)
                        ClientTabControl.TabPages.Add(mOnlyScenarioTab);   //tries to resize

                     Init(mScenarioEditorControl.GetUITarget());


                     mScenarioEditorControl.NewProject(newSizeParams);

                     ClientTabControl.SelectedTab = mOnlyScenarioTab;

                     SetClientFocus();

                     mScenarioEditorControl.PreventDeviceResize = false;

                     loadTEDIntoSceneCrop(path, mScenarioEditorControl);
                     
                     
                  }
               }
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      void loadTEDIntoSceneResize(string scenariofilename, ScenarioEditorPage mScenarioEditorControl,TerrainCreationParams newSizeParams)
      {
         string directory = Path.GetDirectoryName(scenariofilename);
         string tedname = Path.Combine(directory, Path.GetFileNameWithoutExtension(scenariofilename) + ".TED");


         TerrainCreationParams param = new TerrainCreationParams();
         TEDIO.TEDLoadedData dat = new TEDIO.TEDLoadedData();
         TEDIO.mShowAlerts = true;
         TEDIO.LoadTEDvLatest(tedname, ref dat, ref param);

         float oldSizeX = param.mNumVisXVerts * param.mVisTileSpacing;
         float oldSizeZ = param.mNumVisZVerts * param.mVisTileSpacing;

         TEDIO.resampleMap(ref dat, ref param,ref newSizeParams);
         TEDIO.createAllFromTed(dat, newSizeParams);

         //OBJECT DATA!
         TerrainGlobals.getTerrainFrontEnd().LoadScenario(scenariofilename);
         mScenarioEditorControl.LoadProjectSimData(scenariofilename);

         //translate our objects
         SimGlobals.getSimMain().selectAll();
         SimGlobals.getSimMain().forcedObjectRelativeTranslation(oldSizeX, 
                                                                  oldSizeZ, 
                                                                  param.mNumVisXVerts * param.mVisTileSpacing,
                                                                  param.mNumVisZVerts * param.mVisTileSpacing );
         SimGlobals.getSimMain().unselectAll();

         //reset playable bounds since we've resized
         CoreGlobals.mPlayableBoundsMinX = 0;
         CoreGlobals.mPlayableBoundsMinZ = 0;
         CoreGlobals.mPlayableBoundsMaxX = TerrainGlobals.getTerrain().getNumXVerts();
         CoreGlobals.mPlayableBoundsMaxZ = TerrainGlobals.getTerrain().getNumZVerts();
      }
      public void OpenScenarioAsResize(string path, TerrainCreationParams newSizeParams)
      {
         try
         {
            using (PerfSection p = new PerfSection("NewScenario"))
            {
               if (IsSafeToUnloadScenario() || (MessageBox.Show("Are you sure you want to create a new map, you will loose any unsaved data.", "Create new map?", MessageBoxButtons.OKCancel) == DialogResult.OK))
               {
                  using (PerfSection p1 = new PerfSection("resizeBackBuffer()"))
                  {
                     if (mbLoaded == false)
                     {
                        BRenderDevice.resizeBackBuffer(this.Width, this.Height);

                     }
                  }

                  mbLoaded = true;

                  ClientTabPage mOnlyScenarioTab = null;
                  ScenarioEditorPage mScenarioEditorControl = null;

                  if (!containsScenarioTab())
                  {
                     initScenarioTab(ref mScenarioEditorControl, ref mOnlyScenarioTab);
                  }
                  else
                  {
                     giveEditorPage(ref mScenarioEditorControl, ref mOnlyScenarioTab);
                  }


                  using (PerfSection p2 = new PerfSection("othacrap()"))
                  {
                     mScenarioEditorControl.PreventDeviceResize = true;

                     //TabPage scenarioTab = new TabPage("new scenario");            
                     mOnlyScenarioTab.Text = "new scenario";

                     if (ClientTabControl.TabPages.Contains(mOnlyScenarioTab) == false)
                        ClientTabControl.TabPages.Add(mOnlyScenarioTab);   //tries to resize

                     Init(mScenarioEditorControl.GetUITarget());


                     mScenarioEditorControl.NewProject(newSizeParams);

                     ClientTabControl.SelectedTab = mOnlyScenarioTab;

                     SetClientFocus();

                     mScenarioEditorControl.PreventDeviceResize = false;

                     loadTEDIntoSceneResize(path, mScenarioEditorControl, newSizeParams);


                  }
               }
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }



      //QUICK START TAB
      public void NewQuickStart()
      {
         TabPage quickTab = new TabPage("Quick Start");

         QuickStartPage qsp = new QuickStartPage();
         quickTab.Controls.Add(qsp);
         qsp.Dock = DockStyle.Fill;
         ClientTabControl.TabPages.Add(quickTab);
      }


      //PARTICLES TAB
      private ClientTabPage NewParticleTab(ParticleEditor editor)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(editor);
         tabPage.Dock = DockStyle.Fill;
         editor.Dock = DockStyle.Fill;
         return tabPage;
      }
      private void AddParticleTab(ClientTabPage tabPage)
      {
         ClientTabControl.TabPages.Add(tabPage);
         ClientTabControl.SelectedTab = tabPage;
         mParticleEditors.Add(tabPage);
         mActiveTabPage = tabPage;
      }
      public void NewParticleEditor(string filename)
      {
         try
         {
            ParticleEditor particleEditor = new ParticleEditor();
            ClientTabPage tabPage = NewParticleTab(particleEditor);
            if (particleEditor.NewParticleSystem())
            {
               AddParticleTab(tabPage);

               particleEditor.init();
               if (!string.IsNullOrEmpty(filename))
               {
                  particleEditor.open_file(filename);
               }
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }
      public void LoadParticleEditor()
      {
         try
         {
            ParticleEditor particleEditor = new ParticleEditor();
            ClientTabPage tabPage = NewParticleTab(particleEditor);
            if (particleEditor.LoadParticleSystem())
            {
               AddParticleTab(tabPage);
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      //VISUAL TAB
      private ClientTabPage NewVisualTab(VisualEditorPage visualEditor)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(visualEditor);
         tabPage.Dock = DockStyle.Fill;
         return tabPage;
      }
      private void AddVisualTab(ClientTabPage tabPage)
      {
         ClientTabControl.TabPages.Add(tabPage);
         ClientTabControl.SelectedTab = tabPage;
         mVisualEditors.Add(tabPage);
         mActiveTabPage = tabPage;
      }

      public void NewVisualEditor(string filename)
      {
         try
         {
            VisualEditorPage visualEditor = new VisualEditorPage();
            visualEditor.Dock = DockStyle.Fill;

            ClientTabPage tabPage = NewVisualTab(visualEditor);
            visualEditor.SetTabName("Visual Editor");

            AddVisualTab(tabPage);

            visualEditor.init();
            if (!string.IsNullOrEmpty(filename))
            {
               visualEditor.open_file(filename);
            }


            UIManager.InitUIManager(this, visualEditor.GetUITarget());
            UIManager.InitKeyBoardCapture();

            this.ShowDialog("UnitPicker");
            mUnitPicker.DelayedLoad();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }


      //Database TAB
      private ClientTabPage NewDatabaseTab(DatabaseMainPage page)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "Database";
         return tabPage;
      }
      ClientTabPage mDatabaseTab = null;
      public void ShowGameDatabase()
      {
         if (mDatabaseTab == null)
         {
            DatabaseMainPage dbPage = new DatabaseMainPage();
            ClientTabPage tabPage = NewDatabaseTab(dbPage);

            ClientTabControl.TabPages.Add(tabPage);
            ClientTabControl.SelectedTab = tabPage;
            mActiveTabPage = tabPage;
            mDatabaseTab = tabPage;
         }
         else
         {
            ClientTabControl.SelectedTab = mDatabaseTab;
         }
      }

      //Powers Tab
      private ClientTabPage NewPowersEditorTab(PowersPage page)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "New Power";
         return tabPage;
      }   
      public void NewPowersEditor()
      {      
         PowersPage powersPage = new PowersPage();
         ClientTabPage tabPage = NewPowersEditorTab(powersPage);

         ClientTabControl.TabPages.Add(tabPage);
         ClientTabControl.SelectedTab = tabPage;
         mActiveTabPage = tabPage;

      }

      //Script Tab
      private ClientTabPage NewScriptEditorTab(ScriptPage page)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "New Script";
         return tabPage;
      }
      public void NewScriptEditor()
      {
         ScriptPage scriptPage = new ScriptPage();
         ClientTabPage tabPage = NewScriptEditorTab(scriptPage);

         ClientTabControl.TabPages.Add(tabPage);
         ClientTabControl.SelectedTab = tabPage;
         mActiveTabPage = tabPage;

      }
      //Abilities Tab
      private ClientTabPage NewAbilitiesEditorTab(AbilityPage page)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "New Ability";
         return tabPage;
      }
      public void NewAbilityEditor()
      {
         AbilityPage abilitiesPage = new AbilityPage();
         ClientTabPage tabPage = NewAbilitiesEditorTab(abilitiesPage);

         ClientTabControl.TabPages.Add(tabPage);
         ClientTabControl.SelectedTab = tabPage;
         mActiveTabPage = tabPage;

      }

      //Template Tab
      private ClientTabPage NewTriggerTemplateEditorTab(TriggerTemplatePage page)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "New Trigger Template";
         return tabPage;
      }
      public void NewTriggerTemplateEditor()
      {
         TriggerTemplatePage templatePage = new TriggerTemplatePage();
         ClientTabPage tabPage = NewTriggerTemplateEditorTab(templatePage);

         ClientTabControl.TabPages.Add(tabPage);
         ClientTabControl.SelectedTab = tabPage;
         mActiveTabPage = tabPage;

      }


      //Scenario Data Tab
      private ClientTabPage NewScenarioDataTab(ScenarioDataPage page)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "Scenario Data";
         return tabPage;
      }
      public void NewScenarioDataEditor()
      {
         ScenarioDataPage newPage = new ScenarioDataPage();
         ClientTabPage tabPage = NewScenarioDataTab(newPage);

         ClientTabControl.TabPages.Add(tabPage);
         ClientTabControl.SelectedTab = tabPage;
         mActiveTabPage = tabPage;

      }

      //Trigger Definitions
      private ClientTabPage NewTriggerDataDefinitionTab(TriggerDataDefinitionPage page)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "Trigger Definitions";
         return tabPage;
      }
      public void NewTriggerDataDefinitionEditor()
      {
         TriggerDataDefinitionPage newPage = new TriggerDataDefinitionPage();
         ClientTabPage tabPage = NewTriggerDataDefinitionTab(newPage);
         ClientTabControl.TabPages.Add(tabPage);
         ClientTabControl.SelectedTab = tabPage;
         mActiveTabPage = tabPage;
      }

      // Controller configurations
      private ClientTabPage NewControllerConfigurationsTab( ControllerConfigurationsPage page )
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add( page );
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "Controller Configurations";
         return tabPage;
      }
      public void NewControllerConfigurationsEditor()
      {
         ControllerConfigurationsPage newPage = new ControllerConfigurationsPage();
         ClientTabPage tabPage = NewControllerConfigurationsTab( newPage );
         ClientTabControl.TabPages.Add( tabPage );
         ClientTabControl.SelectedTab = tabPage;
         mActiveTabPage = tabPage;
      }

      //String Table Tab
      private ClientTabPage NewStringTableTab(StringTablePage page)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "Stringtable Editor";
         return tabPage;
      }
      public void NewStringTableEditor()
      {
         StringTablePage newPage = new StringTablePage();
         ClientTabPage tabPage = NewStringTableTab(newPage);
         ClientTabControl.TabPages.Add(tabPage);
         ClientTabControl.SelectedTab = tabPage;
         mActiveTabPage = tabPage;
      }


      //HintDatabase Tab
      private ClientTabPage NewHintDatabaseTab(HintDatabasePage page)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "HintDatabase Editor";
         return tabPage;
      }
      public void NewHintDatabaseEditor()
      {
         HintDatabasePage newPage = new HintDatabasePage();
         ClientTabPage tabPage = NewHintDatabaseTab(newPage);
         ClientTabControl.TabPages.Add(tabPage);
         ClientTabControl.SelectedTab = tabPage;
         mActiveTabPage = tabPage;
      }

      //Object Editor Tab
      private ClientTabPage NewObjectEditorTab(ObjectEditorPage page)
      {
         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         page.Dock = DockStyle.Fill;
         tabPage.Text = "ObjectEditor Editor";
         return tabPage;
      }
      public void NewObjectEditor()
      {
         try
         {
            ObjectEditorPage newPage = new ObjectEditorPage();
            ClientTabPage tabPage = NewObjectEditorTab(newPage);
            ClientTabControl.TabPages.Add(tabPage);
            ClientTabControl.SelectedTab = tabPage;
            mActiveTabPage = tabPage;

            newPage.init();

            this.ShowDialog("UnitPicker");
            mUnitPicker.DelayedLoad();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      //OPTIONS TAB
      private ClientTabPage NewOptionsTab(OptionsPage page)
      {

         ClientTabPage tabPage = new ClientTabPage();
         tabPage.Controls.Add(page);
         tabPage.Dock = DockStyle.Fill;
         tabPage.Text = "Options";
         return tabPage;
      }
      ClientTabPage mOptionsTab = null;
      public void ShowScenarioOptions()
      {
         //if (mOptionsTab == null)
         {

            OptionsPage options = new OptionsPage();
            ClientTabPage tabPage = NewOptionsTab(options);

            ClientTabControl.TabPages.Add(tabPage);
            ClientTabControl.SelectedTab = tabPage;
            mParticleEditors.Add(tabPage);
            mActiveTabPage = tabPage;
            mOptionsTab = tabPage;
         }
         //else
         //{
         //   ClientTabControl.SelectedTab = mOptionsTab;
         //}
      }

      //TERRAIN TYPES TAB
      public void initTerrainTypesTab()
      {
         mTerrainTypeEditorControl = new TerrainTypePage();
         mTerrainTypeEditorControl.Dock = DockStyle.Fill;
         mTerrainTypeEditorControl.init();

         mOnlyTerrainTypeTab = new ClientTabPage();
         mOnlyTerrainTypeTab.Controls.Add(mTerrainTypeEditorControl);

         mActiveTabPage = mOnlyTerrainTypeTab;
      }
      public void OpenTerrainTypesEditor()
      {
         try
         {
            if (mTerrainTypeEditorControl == null)
            {
               initTerrainTypesTab();
            }

            if (ClientTabControl.TabPages.Contains(mOnlyTerrainTypeTab) == false)
            {
               mOnlyTerrainTypeTab.Text = "Terrain Types";
               ClientTabControl.TabPages.Add(mOnlyTerrainTypeTab);
            }

            ClientTabControl.SelectedTab = mOnlyTerrainTypeTab;
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      private void MainWindow_Shown(object sender, EventArgs e)
      {
         try
         {
            InitClientTabs();

            // Launch quick start is no command line arguments
            if (!m_bQuickStartVisualEditor &&
               !m_bQuickStartParticleEditor &&
               !m_bQuickStartScenarioEditor &&
               (m_filesToLoad.Count == 0))
            {
               NewQuickStart();
            }

            if (mPreloadErrors.Count > 0)
            {
               ShowErrorOnStatusBar(mPreloadErrors[mPreloadErrors.Count - 1]);
            }



            // Execute command line args
            //

            if (m_bQuickStartVisualEditor)
            {
               NewVisualEditor(null);
            }

            if (m_bQuickStartParticleEditor)
            {
               NewParticleEditor(null);
            }

            if (m_bQuickStartScenarioEditor)
            {
               TerrainCreationParams param = new TerrainCreationParams();
               param.initFromVisData((int)eMapSizes.cMap_Tiny_128v, (int)eMapSizes.cMap_Tiny_128v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
               //param.initFromSimData(127, 127, 1, 2); // micro
               //param.initFromSimData(511, 511, 1, 2); // small

               NewScenario(param);
            }

            
            foreach(string filename in m_filesToLoad)
            {
               open_file_decision(filename);
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }
      #endregion 

      #region InputUpdateRenderLoop
      DateTime lastRender = DateTime.Now;
      //It would be better to place this code in a thread.
      DateTime last = System.DateTime.Now;
      float total = 0;
      int count = 0;

      bool mbEditorPaused = false;
      public void EditorPause()
      {
         mbEditorPaused = true;
      }
      public void EditorUnPause()
      {
         mbEditorPaused = false;
      }
      string getFileDescription()
      {
         float worldX = TerrainGlobals.getTerrain().getNumXVerts() * TerrainGlobals.getTerrain().getTileScale();
         float worldZ = TerrainGlobals.getTerrain().getNumZVerts() * TerrainGlobals.getTerrain().getTileScale();
         //, CoreGlobals.TerrainFile
         string value = " getFileDescription error!";
         try
         {
            if (TerrainGlobals.getEditor() != null && TerrainGlobals.getEditor().getSimRep() != null)
            {

               string name = CoreGlobals.ScenarioFile;
               string dir = CoreGlobals.ScenarioDirectory;
               string dir2 = dir.Replace(CoreGlobals.getWorkPaths().mGameDirectory, "");
               
               value = String.Format("TED: {0} | visVerts: {1}x{2}  | simTiles: {3}x{4}  | WorldSize: {5}x{6}", dir2 + "\\" + name , TerrainGlobals.getTerrain().getNumXVerts(),
                                                                                                                                    TerrainGlobals.getTerrain().getNumZVerts(),
                                                                                                                                    TerrainGlobals.getEditor().getSimRep().getNumXTiles(),
                                                                                                                                    TerrainGlobals.getEditor().getSimRep().getNumXTiles(),
                                                                                                                                    worldX, worldZ);
            }
            else
            {
               value = "";
            }
         }
         catch
         {
         }
         return value;

      }
      bool mbDeviceLost = false;

      private void timer1_Tick(object sender, EventArgs e)
      {
         try
         {
            //Application.
            //if (this.Focused == false)
            //   return;
            //if (this.TopMost == false)
            //   return;


            EditorCore.UIManager.isActiveApp();
            if(mActiveTabPage != null && mActiveTabPage.IsAlive)
               mActiveTabPage.alwaysProcess();

            if (m_bFilesChanged)
            {
               timer1.Enabled = false;


               foreach (string fileName in m_filesToReload)
               {
                  int protoId = BVisualManager.getProtoVisualIndex(fileName); 
                  
                  // Continue if file is not currently loaded
                  if (protoId == -1)
                     continue;

                  bool isLoadedInVisualEditor = false;
                  bool isSavedByEditor = false;

                  // Check all VisualEditorPages to see if the protovisual that was touched
                  // is currently being viewed.  In which case we need to prompt the user to
                  // see if he wants to reload it.
                  //
                  for (int i = 0; i < ClientTabControl.TabPages.Count; i++)
                  {
                     if (ClientTabControl.TabPages[i].Controls[0] is VisualEditorPage)
                     {
                        VisualEditorPage visualEditor = (VisualEditorPage)ClientTabControl.TabPages[i].Controls[0];
                        if (!String.IsNullOrEmpty(visualEditor.visualFileName) && visualEditor.visualFileName.Equals(fileName))
                        {
                           if (visualEditor.visualWasSavedThisFrame)
                           {
                              isSavedByEditor = true;
                              visualEditor.visualWasSavedThisFrame = false;
                           } 
                           
                           isLoadedInVisualEditor = true;
                           break;
                        }
                     }
                  }

                  // If the file was saved by the editor then disregard it
                  if (isSavedByEditor)
                     continue;

                  // Prompt the user
                  if(isLoadedInVisualEditor)
                  {
                     if (MessageBox.Show(this, fileName + "\n\nThis file has been modified outside of the editor.\nDo you want to reload it?", "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.No)
                     {
                        continue;
                     }
                  }

                  // Reload
                  BVisualManager.reloadProtoVisual(fileName);


                  if (isLoadedInVisualEditor)
                  {
                     for (int i = 0; i < ClientTabControl.TabPages.Count; i++)
                     {
                        if (ClientTabControl.TabPages[i].Controls[0] is VisualEditorPage)
                        {
                           VisualEditorPage visualEditor = (VisualEditorPage)ClientTabControl.TabPages[i].Controls[0];
                           if (!String.IsNullOrEmpty(visualEditor.visualFileName) && visualEditor.visualFileName.Equals(fileName))
                           {
                              visualEditor.ReloadVisual();
                           }
                        }
                     }
                  }
               }

               m_filesToReload.Clear();
               m_bFilesChanged = false;

               timer1.Enabled = true;
            }


            if (CoreGlobals.getEditorMain().mOneFrame == true || (mbEditorPaused == false && mActiveTabPage != null && mActiveTabPage.IsAlive))
            {
               CoreGlobals.getEditorMain().mOneFrame = false;
               //if(this.MainWindow_Deactivate == false)
               //{
               //   return;

               //}



               if (BRenderDevice.IsDeviceLost() == true)//mbDeviceLost == false)
               {
                  //mActiveTabPage.render();
               }
               else
               {
                  mActiveTabPage.input();
                  mActiveTabPage.update(); 
                  mActiveTabPage.render();
               }


               count++;
               total += (float)((TimeSpan)(System.DateTime.Now - last)).TotalMilliseconds;
               last = System.DateTime.Now;
               if (count == 5)
               {
                  this.Text = String.Format("{0} {1:F1} fps",getFileDescription() , (5 * 1000f / total));
                  count = 0;
                  total = 0;

               }
               System.Threading.Thread.Sleep(2);
            } 
         }
         catch(Microsoft.DirectX.Direct3D.DeviceLostException devEx)
         {
            mbDeviceLost = true;
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      #endregion

      #region SoundUpdateHandling
      private void soundTimer_Tick(object sender, EventArgs e)
      {
         try
         {
            CoreGlobals.getSoundManager().updateSound();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }
      #endregion

      #region WindowStateHandling

      public void SetClientFocus()
      {
         if (mActiveTabPage != null)
         {
            if (mActiveTabPage.BaseClientPage != null)
            {

               Control c = mActiveTabPage.BaseClientPage.GetUITarget();
               if (c != null)
               {
                  c.Select();
               }
            }
         }
         //panel2.Select();
      }
      private void MainWindow_Enter(object sender, EventArgs e)
      {
         try
         {
            EditorUnPause();
            UIManager.UnPause();
            
            UIManager.InitKeyBoardCapture();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }
      private void MainWindow_Leave(object sender, EventArgs e)
      {
         UIManager.Pause();
         EditorPause();
      }

      private void MainWindow_Activated(object sender, EventArgs e)
      {
         EditorUnPause();
         UIManager.UnPause();

         if (BRenderDevice.IsDeviceLost() == true)
         {
            BRenderDevice.ReCreateDevice();
         }
      }
      protected override void OnResize(EventArgs e)
      {
         if(this.WindowState == FormWindowState.Minimized)
         {
            UIManager.Pause();
            EditorPause();
            TerrainGlobals.getTerrainFrontEnd().Minimize();
         }
         else
         {
            EditorUnPause();
            UIManager.UnPause();

            TerrainGlobals.getTerrainFrontEnd().UnMinimize();

         }
         base.OnResize(e);
      }

      private void MainWindow_Deactivate(object sender, EventArgs e)
      {
         UIManager.Pause();
         EditorPause();
      }
      #endregion

      #region Save and load shit
      private void saveToolStripButton_Click(object sender, EventArgs e)
      {
         file_save_Click(sender, e);
      }

      private void file_save_Click(object sender, EventArgs e)
      {
         try
         {
            EditorPause();
            UIManager.Pause();

            mActiveTabPage.save_file();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            EditorUnPause();
            UIManager.UnPause();
         }
      }

      private void file_saveas_Click(object sender, EventArgs e)
      {
         try
         {
            EditorPause();
            UIManager.Pause();

            mActiveTabPage.save_file_as();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            EditorUnPause();
            UIManager.UnPause();
         }
        
      }


      private void openScenarioToolStripMenuItem_Click(object sender, EventArgs e)
      {
         try
         {
            EditorPause();
            UIManager.Pause();

            OpenProject d = new OpenProject();

            if (d.ShowDialog() == DialogResult.OK)
            {
               if (d.mbDoOpenAs)
               {
                  if(d.mOpenAsCrop)
                     OpenScenarioAsCrop(d.FileName, d.mOpenAsParams);
                  else
                     OpenScenarioAsResize(d.FileName, d.mOpenAsParams);
               }
               else
               {
                  OpenScenario(d.FileName);
               }
            }
            
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            EditorUnPause();
            UIManager.UnPause();
         }
      }

      private void open_file_decision(string filename)
      {
         String ext = Path.GetExtension(filename).ToLower();
         if (ext == @".scn")
         {
            OpenScenario(filename);
         }
         else if (ext == @".vis")
         {
            NewVisualEditor(filename);
         }
         else if (ext == @".pfx")
         {
            NewParticleEditor(filename);
         }
      }

      private void openToolStripButton_Click(object sender, EventArgs e)
      {
         file_open_Click(sender, e);
      }
      private void file_open_Click(object sender, EventArgs e)
      {
         try
         {
            EditorPause();
            UIManager.Pause();

            OpenFileDialog d = new OpenFileDialog();
            d.Filter = "Scenario (*.scn)|*.scn|Visual (*.vis)|*.vis|Particle Effect (*.pfx)|*.pfx";
            if (d.ShowDialog() == DialogResult.OK)
            {
               open_file_decision(d.FileName);
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            EditorUnPause();
            UIManager.UnPause();
         }
      }
    //  NewScenario mNewScenarioDlg = new NewScenario();
      private void customToolStripMenuItem_Click(object sender, EventArgs e)
      {
         //try
         //{
         //   if(mNewScenarioDlg.ShowDialog()==DialogResult.OK)
         //   {
         //      NewScenario(mNewScenarioDlg.mParams);
         //   }
         //}
         //catch (System.Exception ex)
         //{
         //   CoreGlobals.getErrorManager().OnException(ex);
         //}
      }


      #endregion 

      #region Show() for Various Dialogs Code



      public void ShowDialog(string name)
      {
         ShowDialog(name, null, true);
      }


      public void ReloadVisibleTextureThumbnails(bool selectdIndexChanged)
      {
         if(mTexturingPanel != null )
         {
            mTexturingPanel.postTEDLoad(selectdIndexChanged);

         }

      }
      public void roadSelectionCallback()
      {
         if(mRoadPanel != null)
         {
            mRoadPanel.postSelectionCallback();
         }
      }

      void toolStripProgressBar_DoubleClick(object sender, System.EventArgs e)
      {
         FilterDialogs.MemoryEstimateDlg memDlg = new FilterDialogs.MemoryEstimateDlg();
         memDlg.StartPosition = FormStartPosition.CenterScreen;
         memDlg.ShowDialog();
      }

      public void setMemoryEstimateString(string msg)
      {
         toolStripProgressBar.Text = msg;
      }
      public void setTerrainMemoryPercent(float val)
      {
         toolStripProgressBar.Maximum = 100;
         toolStripProgressBar.Minimum = 0;
         int value = (int)(100 * val);
         toolStripProgressBar.Value = value;
         setMemoryEstimateString(value + "%");

         if (val > 0.8f)
            toolStripProgressBar.ForeColor = Color.Red;
         else if (val > 0.5f)
            toolStripProgressBar.ForeColor = Color.Orange;
         else //if (val > 0.5f)
            toolStripProgressBar.ForeColor = Color.Green;
         
      }
      public void doQuickView()
      {
         ScenarioEditorPage mScenarioEditorControl=null;
         ClientTabPage mOnlyScenarioTab=null;
         giveEditorPage(ref mScenarioEditorControl, ref mOnlyScenarioTab);

         mScenarioEditorControl.doQuickView();
        
         afterQuickView();
      }
      public void doQuickSave(string corename)
      {
         ScenarioEditorPage mScenarioEditorControl = null;
         ClientTabPage mOnlyScenarioTab = null;
         giveEditorPage(ref mScenarioEditorControl, ref mOnlyScenarioTab);

         mScenarioEditorControl.doQuickSave(corename);
      }


      public void afterQuickView()
      {
         //reset or we get a crash
         mTexturingPanel.redrawPreviewList(0);
      }


      public void setCursorLocationLabel(float x, float y, float z)
      {
         cursorAtLocLabel.Text = "Cursor : " + x + ", " + y + ", " + z;
      }
      public void clearCursorLocationLabel()
      {
         cursorAtLocLabel.Text = "";
      }

      void ShowDialog(string name, BaseClientPage owner, bool bShared)
      {
         if (name == "UnitPicker")
         {
            ShowUnitPicker();
         }
         if (name == "WorldObjectList")
         {
            ShowWorldObjectList();
         }
         if (name == "ErrorList")
         {
            ShowErrorList();
         }
         if (name == "GlobalLightSettings")
         {
            ShowGlobalLightSettings();
         }
         if (name == "MaskLayers")
         {
            ShowMaskLayers();
         }
         if (name == "BrushSettings")
         {
            ShowBrushSettings();
         }
         if (name == "ClipArtPicker")
         {
            ShowClipArtPicker();
         }
         if (name == "TabletSettings")
         {
            ShowTabletSettings();
         }
         if(name == "NoiseSettings")
         {
            ShowNoiseSettings();
         }
         if (name == "TexturingPanel")
         {
            ShowTexturingPanel();
         }
         if (name == "WaterPanel")
         {
            ShowWaterPanel();
         }
         if (name == "RoadPanel")
         {
            ShowRoadPanel();
         }
         if (name == "FoliagePanel")
         {
            ShowFoliagePanel();
         }
         if (name == "DesignerObjectsPanel")
         {
            ShowDesignerObjectsPanel();
         }

         this.SetClientFocus();
      }

      public BaseClientPage getActiveClientPage()
      {
         return (mActiveTabPage != null) ? mActiveTabPage.BaseClientPage : null;
      }

      //called externally for other panels to group too.
      public void dockToGrouping(Xceed.DockingWindows.ToolWindow panel)
      {

         if (mWorldObjectList != null)
         {
            panel.DockTo(mWorldObjectList, DockPosition.Group);
         }
         else
         {
            panel.DockTo(DockTargetHost.DockHost, DockPosition.Left);
         }
         panel.Width = 250;      
      }

      public TexturePaneFull mTexturingPanel = null;
      private void ShowTexturingPanel()
      {
         try
         {
            if (mTexturingPanel == null)
            {

               mTexturingPanel = new TexturePaneFull();
               mDockLayoutManager.ToolWindows.Add(mTexturingPanel);
              // mTexturingPanel.DockLayoutManager.AllowDocking = false;
             //  mDockLayoutManager.AllowDocking = false;

               dockToGrouping(mTexturingPanel);
            }
            else
            {
               mTexturingPanel.Show();
               mDockLayoutManager.ToolWindows["TexturePaneFull"].Activate();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      public WaterPanel mWaterPanel = null;
      private void ShowWaterPanel()
      {
         try
         {
            if (mWaterPanel == null)
            {

               mWaterPanel = new WaterPanel();
               mDockLayoutManager.ToolWindows.Add(mWaterPanel);

               dockToGrouping(mWaterPanel);
            }
            else
            {
               mWaterPanel.Show();
               mDockLayoutManager.ToolWindows["WaterPanel"].Activate();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      public RoadPanel mRoadPanel = null;
      private void ShowRoadPanel()
      {
         try
         {
            if (mRoadPanel == null)
            {

               mRoadPanel = new RoadPanel();
               mDockLayoutManager.ToolWindows.Add(mRoadPanel);

               dockToGrouping(mRoadPanel);
            }
            else
            {
               mRoadPanel.Show();
               mDockLayoutManager.ToolWindows["RoadPanel"].Activate();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }
      public FoliagePanel mFoliagePanel = null;
      private void ShowFoliagePanel()
      {
         try
         {
            if (mFoliagePanel == null)
            {

               mFoliagePanel = new FoliagePanel();
               mDockLayoutManager.ToolWindows.Add(mFoliagePanel);

               dockToGrouping(mFoliagePanel);
            }
            else
            {
               mFoliagePanel.Show();
               mDockLayoutManager.ToolWindows["FoliagePanel"].Activate();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      public DesignerObjectsPanel mDesignerObjectsPanel = null;
      private void ShowDesignerObjectsPanel()
      {
         try
         {
            if (mDesignerObjectsPanel == null)
            {

               mDesignerObjectsPanel = new DesignerObjectsPanel();
               mDockLayoutManager.ToolWindows.Add(mDesignerObjectsPanel);

               dockToGrouping(mDesignerObjectsPanel);
            }
            else
            {
               mDesignerObjectsPanel.Show();
               mDockLayoutManager.ToolWindows["DesignerObjectsPanel"].Activate();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      public PropertiesWindow mBrushSettingsWindow = null;
      private void ShowBrushSettings()
      {
         try
         {
            if (mBrushSettingsWindow == null)
            {

               mBrushSettingsWindow = new PropertiesWindow();
               //mBrushSettingsWindow.Show();
               mDockLayoutManager.ToolWindows.Add(mBrushSettingsWindow);

               //mBrushSettingsWindow.DockLayoutManager.AllowDocking = false;
               //mDockLayoutManager.AllowDocking = false;
               //mBrushSettingsWindow.DockTo(Xceed.DockingWindows.DockTargetHost.ClientHost , Xceed.DockingWindows.DockPosition.Right);

               dockToGrouping(mBrushSettingsWindow);
            }
            else
            {
               mBrushSettingsWindow.Show();
               mDockLayoutManager.ToolWindows["Properties"].Activate();

            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      TabletSettings mTabletSettings = null;
      private void ShowTabletSettings()
      {
        try
         {
            if (mTabletSettings == null)
            {

               mTabletSettings = new TabletSettings();
               mDockLayoutManager.ToolWindows.Add(mTabletSettings);
               //mTabletSettings.DockLayoutManager.AllowDocking = false;
            }
            else
            {
               mTabletSettings.Show();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      NoiseSettings mNoiseSettings = null;
      private void ShowNoiseSettings()
      {
         try
         {
            if (mNoiseSettings == null)
            {

               mNoiseSettings = new NoiseSettings();
               mDockLayoutManager.ToolWindows.Add(mNoiseSettings);
               //mNoiseSettings.DockLayoutManager.AllowDocking = false;
            }
            else
            {
               mNoiseSettings.Show();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      ClipArtPicker mClipArtPicker = null;
      private void ShowClipArtPicker()
      {
         try
         {
            if (mClipArtPicker == null)
            {
               mClipArtPicker = new ClipArtPicker();
               mDockLayoutManager.ToolWindows.Add(mClipArtPicker);

               if (mUnitPicker != null)
               {
                  mDockLayoutManager.ToolWindows["ClipArtPicker"].DockTo(mUnitPicker, DockPosition.Group);
               }
               if (mWorldObjectList != null)
               {
                  mDockLayoutManager.ToolWindows["ClipArtPicker"].DockTo(mWorldObjectList, DockPosition.Group);
               }
               else
               {
                  mDockLayoutManager.ToolWindows["ClipArtPicker"].DockTo(DockTargetHost.DockHost, DockPosition.Left);
               }


            }
            else
            {
               mClipArtPicker.Show();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      public UnitPicker mUnitPicker = null;
      private void ShowUnitPicker()
      {
         try
         {
            EditorPause();
            UIManager.Pause();

            if (mUnitPicker == null)
            {
               mUnitPicker = new UnitPicker();
               mDockLayoutManager.ToolWindows.Add(mUnitPicker);
               //mUnitPicker.DockTo(mDockLayoutManager.ClientHost, Xceed.DockingWindows.DockPosition.Right);
               //mDockLayoutManager.AllowDocking = false;

               if (mWorldObjectList != null)
               {
                  mDockLayoutManager.ToolWindows["UnitPicker"].DockTo(mWorldObjectList, DockPosition.Group);
               }
               else
               {

                  mDockLayoutManager.ToolWindows["UnitPicker"].DockTo(DockTargetHost.DockHost, DockPosition.Left);
               }
            }
            else
            {
               
               mUnitPicker.Show();
               mDockLayoutManager.ToolWindows["UnitPicker"].Activate();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            EditorUnPause();
            UIManager.UnPause();
         }     
      }
      MaskLayers mMaskLayers = null;
      private void ShowMaskLayers()
      {
         try
         {
            EditorPause();
            UIManager.Pause();

            if (mMaskLayers == null)
            {
               mMaskLayers = new MaskLayers();
               mDockLayoutManager.ToolWindows.Add(mMaskLayers);
               //mUnitPicker.DockTo(mDockLayoutManager.ClientHost, Xceed.DockingWindows.DockPosition.Right);
               //mDockLayoutManager.AllowDocking = false;

               CoreGlobals.getEditorMain().Register(mMaskLayers);//make this more generic?

               if (mUnitPicker != null)
               {
                  mDockLayoutManager.ToolWindows["MaskLayers"].DockTo(mUnitPicker, DockPosition.Group);
               }
               if (mWorldObjectList != null)
               {
                  mDockLayoutManager.ToolWindows["MaskLayers"].DockTo(mWorldObjectList, DockPosition.Group);
               }
               else
               {
                  mDockLayoutManager.ToolWindows["MaskLayers"].DockTo(DockTargetHost.DockHost, DockPosition.Left);
               }
            }
            else
            {
               mMaskLayers.Show();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            EditorUnPause();
            UIManager.UnPause();
         }
      }
      WorldObjectList mWorldObjectList = null;
      private void ShowWorldObjectList()
      {
         try
         {
            EditorPause();
            UIManager.Pause();

            if (mWorldObjectList == null)
            {
               mWorldObjectList = new WorldObjectList();
               mDockLayoutManager.ToolWindows.Add(mWorldObjectList);
               //mUnitPicker.DockTo(mDockLayoutManager.ClientHost, Xceed.DockingWindows.DockPosition.Right);
               //mDockLayoutManager.AllowDocking = false;

               if (mUnitPicker != null)
               {
                  mDockLayoutManager.ToolWindows["WorldObjectList"].DockTo(mUnitPicker, DockPosition.Group);
               }
               else
               {
                  mDockLayoutManager.ToolWindows["WorldObjectList"].DockTo(DockTargetHost.ClientHost, DockPosition.Left);
               }
            }
            else
            {
               
               mWorldObjectList.Show();
               mDockLayoutManager.ToolWindows["WorldObjectList"].Activate();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            EditorUnPause();
            UIManager.UnPause();
         }
      }
      ErrorList mErrorList = null;
      private void ShowErrorList()
      {
         
         try
         {
            EditorPause();
            UIManager.Pause();

            if (mErrorList == null)
            {
               mErrorList = new ErrorList();
               mDockLayoutManager.ToolWindows.Add(mErrorList);
               //mDockLayoutManager.ToolWindows["ErrorList"].DockTo(DockTargetHost.ClientHost, DockPosition.Bottom);
              
               foreach(string text in mPreloadErrors)
               {
                  mErrorList.UpdateList(text);
               }
            }
            else 
            {

               mErrorList.Show();
               mDockLayoutManager.ToolWindows["ErrorList"].Activate();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            EditorUnPause();
            UIManager.UnPause();
         }
      }
     
      ProtoObjectQuickEditor mProtoObjectQuickEditor = null;
      private void ShowProtoObjectWizard()
      {
         if (mProtoObjectQuickEditor == null || mProtoObjectQuickEditor.IsDisposed)
            mProtoObjectQuickEditor = new ProtoObjectQuickEditor();
         mProtoObjectQuickEditor.Show();
      }
      TempLightSettings mTempLightSettings = null;
      public void ShowGlobalLightSettings()
      {
         if (mTempLightSettings == null || mTempLightSettings.IsDisposed)
            mTempLightSettings = new TempLightSettings();
         mTempLightSettings.Show();

         if (!mEditorWindows.Contains(mTempLightSettings))
         {
            mEditorWindows.Add(mTempLightSettings);
         }
      }

      #endregion 
 
      #region Error List stuff
      List<string> mPreloadErrors = new List<string>();
      public void AddToErrorList(string text)
      {
        
         if (mErrorList == null)
         {
            mPreloadErrors.Add(text);
         }
         //   ShowErrorList();

         if (mErrorList != null)
            mErrorList.UpdateList(text);
         ShowErrorOnStatusBar( text );
      }

      public void ShowErrorOnStatusBar(string text)
      {
         this.LastErrorToolStripStatusLabel.Text = text.Substring(0, Math.Min(120, text.Length)).Replace("\r", " ").Replace("\n", " ") + "...(click to view details)";

      }
      private void LastErrorToolStripStatusLabel_Click(object sender, EventArgs e)
      {
         ShowErrorList();
      }
      #endregion

      #region Main Menu Stuff
      private void optionsToolStripMenuItem_Click_1(object sender, EventArgs e)
      {
         ShowScenarioOptions();
      }
      private void errorListToolStripMenuItem_Click(object sender, EventArgs e)
      {
         ShowErrorList();
      }
      private void protoObjectsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         ShowProtoObjectWizard();
      }

      private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
      {
         string XTDVersion = "Editor Version  " + CoreGlobals.getVersion() +
                              "\nXSD Version  " + eFileVersions.cXSDVersion + 
                               "\nXTD Version " + eFileVersions.cXTDVersion ;
         MessageBox.Show(XTDVersion);
      }
      #endregion

      #region TabPage Management

      List<IEditorWindow> mEditorWindows = new List<IEditorWindow>();
      private bool mbSuspendToolbar = false;
      private void ClientTabControl_Selected(object sender, TabControlEventArgs e)
      {
         ClientTabPage page = e.TabPage as ClientTabPage;
         if (page != null)
         {
            ActivatePage(page);
         }
      }

      private void ClientTabControl_ControlAdded(object sender, ControlEventArgs e)
      {
         // The first time a control is added we need to activate it since the 'Selected' event will
         // not be called (since it gets selected when is added).  So here we catch the first tab
         // page added and we activate it.
         //

         if (ClientTabControl.Controls.Count > 1)
            return;

         ClientTabPage page = e.Control as ClientTabPage;
         if (page != null)
         {
            ActivatePage(page);
         }
      }

      private void ActivatePage(ClientTabPage page)
      {
         toolStripContainer1.SuspendLayout();
         toolStripContainer1.TopToolStripPanel.SuspendLayout();
         mbSuspendToolbar = true;

         if (mActiveTabPage != null)
         {
            mActiveTabPage.Deactivate();
            UnLoadClientDynamic(mActiveTabPage.BaseClientPage.DynamicMenus);
         }


         page.Activate();
         mActiveTabPage = page;
         LoadClientDynamic(mActiveTabPage.BaseClientPage.DynamicMenus);

         if (page.BaseClientPage.GetUITarget() != null)
         {
            UIManager.InitUIManager(this, page.BaseClientPage.GetUITarget());
            UIManager.InitKeyBoardCapture();
         }


         mbSuspendToolbar = false;
         toolStripContainer1.TopToolStripPanel.ResumeLayout();
         toolStripContainer1.ResumeLayout();

         this.Invalidate();

         SetClientFocus();
      }

      private ToolStripMenuItem GetMenuByName(string name)
      {
         foreach (ToolStripMenuItem menu in MainMenuStrip.Items)
         {
            if (menu.Text == name)
               return menu;
         }
         return null;
      }

      private void LoadClientDynamic(DynamicMenus dynamicMenus)
      {
         int count = MainMenuStrip.Items.Count;
         
         int mainOffset = 0;
         foreach(ToolStripMenuItem menu in dynamicMenus.mMenus)
         {
            
            ToolStripMenuItem topLevelMenu = GetMenuByName(menu.Text);

            if (topLevelMenu == null)
            {
               MainMenuStrip.Items.Insert(mainOffset + count - 1, menu);
               mainOffset++;
            }
            else
            {
               ToolStripItem exit = null;
               if (topLevelMenu.Text == "File")
               {
                  exit = topLevelMenu.DropDownItems[topLevelMenu.DropDownItems.Count - 1];
                  topLevelMenu.DropDownItems.Remove(exit);
               }    
      
               int numItems = menu.DropDownItems.Count;
               List<ToolStripItem> subItems = (List<ToolStripItem>)menu.Tag;
               for (int i = 0; i < subItems.Count; i++)
               {
                  //subItems[i].Visible = true;
                  if (topLevelMenu.DropDownItems.Contains(subItems[i]) == false)
                  {
                     topLevelMenu.DropDownItems.Add(subItems[i]);
                     subItems[i].Tag = "temporary";  //used for removing this later.
                  }                 
               }  
               if(exit != null)
               {
                  topLevelMenu.DropDownItems.Add(exit);
               }
            }

         }
         if (!mbSuspendToolbar)
         {
            toolStripContainer1.SuspendLayout();
            toolStripContainer1.TopToolStripPanel.SuspendLayout();
         }
         foreach (ToolStrip toolStrip in dynamicMenus.mToolStrips)
         {
            if (toolStrip.Name.Contains( "DummyToolStrip")) 
               continue;
            if (toolStripContainer1.TopToolStripPanel.Controls.Contains(toolStrip))
               continue;
            toolStripContainer1.TopToolStripPanel.Controls.Add(toolStrip);            
         }
         if (!mbSuspendToolbar)
         {
            toolStripContainer1.TopToolStripPanel.ResumeLayout();
            toolStripContainer1.ResumeLayout();
         }
      }
      private void UnLoadClientDynamic(DynamicMenus dynamicMenus)
      {
         foreach (ToolStripMenuItem menu in dynamicMenus.mMenus)
         {

            if (MainMenuStrip.Items.Contains(menu) == true)
               MainMenuStrip.Items.Remove(menu);
         }
         foreach (ToolStripMenuItem menu in MainMenuStrip.Items)
         {
            List<ToolStripItem> toRemove = new List<ToolStripItem>();
            foreach (ToolStripItem t in menu.DropDownItems)
            {
               if(t.Tag is string)
               {
                  toRemove.Add(t);
               }
            }
            foreach (ToolStripItem t in toRemove)
            {
               menu.DropDownItems.Remove(t);
            }
    
         }
         if (!mbSuspendToolbar)
         {
            toolStripContainer1.SuspendLayout();
            toolStripContainer1.TopToolStripPanel.SuspendLayout();
         }
         foreach (ToolStrip toolStrip in dynamicMenus.mToolStrips)
         {
            if (toolStripContainer1.TopToolStripPanel.Controls.Contains(toolStrip) == true)
               toolStripContainer1.TopToolStripPanel.Controls.Remove(toolStrip);           
         }
         if (!mbSuspendToolbar)
         {
            toolStripContainer1.TopToolStripPanel.ResumeLayout();
            toolStripContainer1.ResumeLayout();
         }
      }

      private void closeActiveTab_Click(object sender, EventArgs e)
      {
         int k = ClientTabControl.SelectedIndex;
         if (k == 0) //we can't close quickstart page
            return;

         ClientTabPage page = ClientTabControl.TabPages[k] as ClientTabPage;

         UnLoadClientDynamic(page.BaseClientPage.DynamicMenus);
         page.destroyPage();
         ClientTabControl.TabPages.RemoveAt(k);
      }

      //This should stay small, we may change the type of tab control used to something better.
      sealed private class ClientTabPage : TabPage
      {
         public BaseClientPage BaseClientPage
         {
            get
            {
               foreach (Control c in this.Controls)
               {
                  BaseClientPage b = c as BaseClientPage;
                  if (b != null)
                  {
                     return b;
                  }
               }
               return null;
            }
         }


         
         public void Activate()
         {
            //These for loops need to be obliterated and replaced with something more direct.  my bad
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.Activate();
               }
            }
         }
         public void Deactivate()
         {
            //These for loops need to be obliterated and replaced with something more direct.  my bad
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.Deactivate();
               }
            }
         }
         bool mbDestroyed = false;
         public void destroyPage()
         {
            mbDestroyed = true;
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.destroyPage();
               }
            }
         }
         public bool IsAlive
         {
            get
            {
               return !mbDestroyed;
            }
         }

         public void init()
         {
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.init();
               }
            }
         }
         public void deinit()
         {
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.deinit();
               }
            }
         }
         public void input()
         {
            if (mbDestroyed) return;
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.input();
               }
            }
         }

         public void alwaysProcess()
         {
            if (mbDestroyed) return;

            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.alwaysProcess();
               }
            }
         }

         public void update()
         {
            if (mbDestroyed) return;

            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.update();
               }
            }
         }
         public void render()
         {
            if (mbDestroyed) return;

            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.render();
               }
            }
         }
         public void initDeviceData()
         {
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.initDeviceData();
               }
            }
         }
         public void deinitDeviceData()
         {
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.deinitDeviceData();
               }
            }
         }
         public void save_file()
         {
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.save_file();
               }
            }
         }
         public void save_file_as()
         {
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.save_file_as();
               }
            }
         }
         public void open_file(string filename)
         {
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.open_file(filename);
               }
            }
         }
         public void open_file()
         {
            foreach (Control c in this.Controls)
            {
               BaseClientPage b = c as BaseClientPage;
               if (b != null)
               {
                  b.open_file();
               }
            }
         }

      }

      #endregion

      private void MainWindow_DragDrop(object sender, DragEventArgs e)
      {
         // transfer the filenames to a string array
         string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

         // loop through the string array, opening each file
         foreach (string filename in files)
         {
            open_file_decision(filename);
         }
      }

      private void MainWindow_DragEnter(object sender, DragEventArgs e)
      {
         // Make sure we are dropping files (not text or anything else)
         if (e.Data.GetDataPresent(DataFormats.FileDrop, false) == true)
         {
            // Check to see if at least one file in the drop list is valid
            bool atLeastOneFileIsValid = false;
            string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

            foreach (string filename in files)
            {
               String ext = Path.GetExtension(filename).ToLower();
               if (ext == @".scn")
               {
                  atLeastOneFileIsValid = true;
                  break;
               }
               else if (ext == @".vis")
               {
                  atLeastOneFileIsValid = true;
                  break;
               }
               else if (ext == @".pfx")
               {
                  atLeastOneFileIsValid = true;
                  break;
               }
            }

            if (atLeastOneFileIsValid)
               e.Effect = DragDropEffects.All;
         }
      }



     



      private void FileWatcher_Changed(object sender, FileSystemEventArgs e)
      {
         string ext = Path.GetExtension(e.Name);

         if (string.Compare(ext, ".vis", true) == 0)
         {
            // Check if file is loaded
            int protoId = BVisualManager.getProtoVisualIndex(e.FullPath);

            if (protoId != -1)
            {
               if (!m_filesToReload.Contains(e.FullPath))
                  m_filesToReload.Add(e.FullPath);

               m_bFilesChanged = true;
            }
         }
      }

      private void importGR2ToolStripMenuItem_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();
         //d.Filter = "Granny File (*.gr2)|*.gr2|Effect (*.pfx)|*.pfx";
         d.Filter = "Granny File (*.gr2)|*.gr2|Effect (*.pfx)|*.pfx|Lightset (*.lgt)|*.lgt";
         d.FilterIndex = 0;
         d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameArtDirectory;

         if (d.ShowDialog() == DialogResult.OK)
         {
            //if(d.FileName.Contains(CoreGlobals.getWorkPaths().mGameArtDirectory) == false)
            if (d.FileName.StartsWith(CoreGlobals.getWorkPaths().mGameArtDirectory, StringComparison.OrdinalIgnoreCase) == false)
            {
               MessageBox.Show("GR2 must be in the art directory.  Does this suck? maybe we could let you type a folder in and do and auto move");
               return;
            }

            GR2ObjectImporter mover = new GR2ObjectImporter(d.FileName);

            if (mover.ShowDialog() == DialogResult.OK)
            {

            }
         }
      }

      private void importSoundsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         SoundObjectImporter soundImport = new SoundObjectImporter();
         if (soundImport.ShowDialog() == DialogResult.OK)
         {

         }     
      }
   }


   public interface IEditorWindow
   {
      void reload();
   }
}
