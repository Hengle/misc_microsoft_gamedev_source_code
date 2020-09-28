using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Collections;
using System.Threading;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using Terrain;
using UIHelper;
using SimEditor;
using System.IO; 
using EditorCore;
using Rendering;
using ModelSystem;
using Export360;
using EditorCore.Controls.Micro;
using System.Xml.Serialization;

using PhoenixEditor.ScenarioEditor;

namespace PhoenixEditor.ClientTabPages
{
   public partial class ScenarioEditorPage : EditorCore.BaseClientPage, IUnitPicker, IPhoenixScenarioEditor, UiGenerator.IToolBarCommandHandler, ISquadPicker, IGraphicsWindowOwner
   {
      //Starting to move to local variables instead of singletons to prepare for having multiple scenarios open
      BTerrainFrontend mTerrainEdit;
      SimMain mSimMain;

      public ScenarioEditorPage()
      {
         using (PerfSection p = new PerfSection("new ScenarioEditorPage"))
         {

            ProgressHelper.StatusMessage("ScenarioEditorPage + InitializeComponent");


            InitializeComponent();

            RenderModeToolStripComboBox.SelectedIndex = 0;

            ProgressHelper.StatusMessage("BuildDynamicMenus");
            mDynamicMenus.BuildDynamicMenus(toolStripContainer1, menuStrip1);


            //   startActiveSyncToolStripMenuItem.Visible = false;

            CoreGlobals.getEditorMain().Register(this);


            ProgressHelper.StatusMessage("ScenarioEditorPage()-finished");

            RedoToolStripButton.Enabled = true;
            RedoToolStripButton.Click+=new EventHandler(RedoToolStripButton_Click);

            //importGR2ToolStripMenuItem.Enabled = false;



            this.SettingsTabPage.Controls.Add(mPlayerSettings);
            mPlayerSettings.Dock = DockStyle.Fill;

            this.ObjectivesTabPage.Controls.Add(mObjectivesControl);
            mObjectivesControl.Dock = DockStyle.Fill;

            this.GlobalLightsTabPage.Controls.Add(mTempLightSettings);
            mTempLightSettings.Dock = DockStyle.Fill;



            this.ScriptsTabPage.Controls.Add(mlScenarioScriptsManager);
            mlScenarioScriptsManager.Dock = DockStyle.Fill;
            mlScenarioScriptsManager.mParentTabs = TabControl;
            mlScenarioScriptsManager.mBaseClientParent = this;

            //this.TriggersTabPage.Controls.Add(mFirstTriggerEditor);
            //mFirstTriggerEditor.Dock = DockStyle.Fill;
            //mFirstTriggerEditor.mBaseClientParent = this;
            //TriggersTabPage.Tag = mFirstTriggerEditor;

            this.scenarioDataTabPage.Controls.Add(mScenarioDataSettings);
            mScenarioDataSettings.Dock = DockStyle.Fill;

            MapTabPage.Tag = this;


            showFogToolStripMenuItem.Checked = CoreGlobals.getSettingsFile().ShowFog;
            TerrainGlobals.getEditor().setRenderFog(CoreGlobals.getSettingsFile().ShowFog);

            designerControlsToolStripMenuItem.Checked = CoreGlobals.getSettingsFile().DesignerControls;
            SimGlobals.getSimMain().useDesignerControls(CoreGlobals.getSettingsFile().DesignerControls);

            artistModPanToolStripMenuItem.Checked = CoreGlobals.getSettingsFile().ArtistModePan;

            CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics().Clear();
            CoreGlobals.getScenarioWorkTopicManager().mChangeListPrefix = "Scenario: ";
            CoreGlobals.getScenarioWorkTopicManager().TopicsUpdated += new EventHandler(ScenarioEditorPage_TopicsUpdated);

            timer1.Enabled = CoreGlobals.getSettingsFile().AutoSaveEnabled;
            timer1.Interval = CoreGlobals.getSettingsFile().AutoSaveTimeInMinutes * 60000;
            timer2.Interval = 600000;

            using (PerfSection p2 = new PerfSection("mScenarioSourceControl"))
            {
               this.PerforceTabPage.Controls.Add(mScenarioSourceControl2);
               mScenarioSourceControl2.Dock = DockStyle.Fill;
               //Label l = new Label();
               //l.Text = "This page has been removed temporarily.  Please use perforce to check out / in the files";
               //this.PerforceTabPage.Controls.Add(l);
            }

            randomGenToolStripMenuItem.Enabled = CoreGlobals.IsDev;
         }
      }

      void ScenarioEditorPage_TopicsUpdated(object sender, EventArgs e)
      {
         Dictionary<string, WorkTopic> workTopics = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics();
         WorkTopic simTopic = workTopics["Sim"];
         WorkTopic artObjTopic = workTopics["ArtObjects"];
         WorkTopic soundsTopic = workTopics["Sounds"];

         List<WorkTopic> testMe = new List<WorkTopic>();
         testMe.Add(simTopic);
         testMe.Add(artObjTopic);
         testMe.Add(soundsTopic);

         int valid = 0;
         foreach (WorkTopic w in testMe)         
         {
            if (w.mState == WorkTopic.eWorkTopicFileState.cLocalWriteable)
            {
               valid++;
            }
            else if (w.mState == WorkTopic.eWorkTopicFileState.cCheckedOutByUser)
            {
               valid++;
            }
            else if (w.mState == WorkTopic.eWorkTopicFileState.cNoFiles)
            {
               valid++;
            }
         }
         if (valid == testMe.Count)
         {
            //Allow group changing
            mSimMain.AllowDepartmentChanges = true;

         }
         else
         {
            //Don't allow group changing
            mSimMain.AllowDepartmentChanges = false;

         }
      }

      

      //List<TriggerEditor> mTriggerEditors = new List<TriggerEditor>();

      //TriggerEditor mFirstTriggerEditor = new TriggerEditor();
      //ScenarioSourceControl mScenarioSourceControl = new ScenarioSourceControl();
      ScenarioSourceControl2 mScenarioSourceControl2 = new ScenarioSourceControl2();

      TempLightSettings mTempLightSettings = new TempLightSettings();
      ScenarioScriptsManager mlScenarioScriptsManager = new ScenarioScriptsManager();
      PlayerSettings mPlayerSettings = new PlayerSettings();
      ObjectivesControl mObjectivesControl = new ObjectivesControl();
      ScenarioSettings mScenarioDataSettings = new ScenarioSettings();

      public Control GetGraphicsWindow()
      {
         return this.panel1;
      }
      public bool GraphicsEnabled()
      {
         return true;
      }

      public void reloadActiveTextureBar()
      {
       
      }
      public void InitToolBars()
      {
         UiGenerator gen = new UiGenerator();

         //Build a command list from the main terrain class
         ArrayList terrainCommands = gen.GetCommandsFromObject(mTerrainEdit, this);
         SimToolStrip.Items.AddRange((ToolStripButton[])(gen.GetToolStripButtons(terrainCommands, mTerrainEdit).ToArray(typeof(ToolStripButton))));
         terrainToolStripMenuItem.DropDownItems.Add(new ToolStripSeparator());
         terrainToolStripMenuItem.DropDownItems.AddRange((ToolStripItem[])gen.GetMenuButtons(terrainCommands, mTerrainEdit).ToArray(typeof(ToolStripItem)));

         //Build a command list from the main sim class
         ArrayList simCommands = gen.GetCommandsFromObject(mSimMain, this);
         SimToolStrip.Items.AddRange((ToolStripButton[])(gen.GetToolStripButtons(simCommands,mSimMain).ToArray(typeof(ToolStripButton))));
         simOptionsToolStripMenuItem.DropDownItems.Add(new ToolStripSeparator());
         simOptionsToolStripMenuItem.DropDownItems.AddRange((ToolStripItem[])gen.GetMenuButtons(simCommands, mSimMain).ToArray(typeof(ToolStripItem)));






         CurrentCommandOptionsToolStrip.MinimumSize = new Size(300, 27);
         LoadToolStripCommandOptions();

      }

      #region

      //this is not ideal, the scroll strip class should be reworked.
      public void UpdateSliders()
      {
         //CurrentCommandOptionsToolStrip.Items.AddRange(mToolstripCommandOptions[name].ToArray());

         foreach (object t in CurrentCommandOptionsToolStrip.Items)
         {
            if (t is ScrollStripItem)
            {
               ScrollStripItem s = t as ScrollStripItem;
               if(s.ValueName == "Intensity")
               {
                  s.Value = mTerrainEdit.VertBushIntensity;
               }
               if (s.ValueName == "Size")
               {
                  s.Value = mTerrainEdit.VertBrushRadius;
               }
               if (s.ValueName == "Hotspot")
               {
                  s.Value = mTerrainEdit.VertBrushHotspot;
               }
               if (s.ValueName == "Rotation")
               {
                  s.Value = mTerrainEdit.TexBrushRotation;
               }
            }
            else if (t is ToolStripButton)
            {
               //ToolStripButton b = t as ToolStripButton;
               //if (b.CheckOnClick == true)
               //{
               //   b.Checked = b.Checked;

               //}
            }


         }

      }


      Dictionary<string, List<ToolStripItem>> mToolstripCommandOptions = new Dictionary<string, List<ToolStripItem>>();

      //todo... make this data driven??
      private void LoadToolStripCommandOptions()
      {
         mToolstripCommandOptions = new Dictionary<string, List<ToolStripItem>>();
         List<ToolStripItem> commandList = new List<ToolStripItem>();

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         commandList.Add(MakeNoiseCheckButton());
         commandList.Add(MakeSphereOrCylinderIntersectionSelectionButton());
         mToolstripCommandOptions["Height Only Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         commandList.Add(MakeNoiseCheckButton());
         mToolstripCommandOptions["Normal Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Layer Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         commandList.Add(MakeNoiseCheckButton()); 
         commandList.Add(MakeSphereOrCylinderIntersectionSelectionButton());
         mToolstripCommandOptions["Set Height Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Inflate Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Pinch Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Smooth Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         commandList.Add(MakeSmudgeBrushCombo());
         mToolstripCommandOptions["Smudge Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         commandList.Add(MakeNoiseCheckButton());
         mToolstripCommandOptions["Push Pull Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Uniform Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         commandList.Add(MakeScaleBrushModesCombo());
         mToolstripCommandOptions["Scalar Brush"] = commandList;


         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Skirt Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Alpha Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeTesselationModesCombo());
         commandList.Add(makeTessellationClearButton());
         mToolstripCommandOptions["Tessellation Brush"] = commandList;
         
         
         commandList = new List<ToolStripItem>();
         /*
         commandList.Add(MakeIntensitySlider());
       //  commandList.Add(MakeSizeSlider());
       //  commandList.Add(MakeRotationSlider());
         commandList.Add(MakeRotateClipModesCombo());
         commandList.Add(MakeOKPaste());
         commandList.Add(MakeCancelPaste());         
         */
         mToolstripCommandOptions["Paste Terrain"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeRotationSlider());
         commandList.Add(MakeIntensitySlider(1.0f));
         mToolstripCommandOptions["Texture Paint"] = commandList;


         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider(1.0f));
         commandList.Add(MakeImageSelectionButton());
         mToolstripCommandOptions["Shape Select Mode"] = commandList;


         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Sim Heights Override"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Sim Set Height Override"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakePassableModesCombo());
         commandList.Add(makePassableClearButton());
         mToolstripCommandOptions["Passable Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeBuildableModesCombo());
         commandList.Add(makeBuildableClearButton());
         mToolstripCommandOptions["Buildable Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeFloodPassableModesCombo());
         commandList.Add(makeFloodPassableClearButton());
         mToolstripCommandOptions["FloodPassable Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeScarabPassableModesCombo());
         commandList.Add(makeScarabPassableClearButton());
         mToolstripCommandOptions["ScarabPassable Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeTileTypeModesCombo());
         commandList.Add(MakeTileTypesCombo());
         commandList.Add(makeTileTypeClearButton());
         mToolstripCommandOptions["TileType Brush"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Foliage Paint"] = commandList;


         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Camera Heights Override"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Camera Set Heights Override"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Camera Smooth Heights Override"] = commandList;

         commandList = new List<ToolStripItem>();
         commandList.Add(MakeIntensitySlider());
         commandList.Add(MakeSizeSlider());
         commandList.Add(MakeHotspotSlider());
         mToolstripCommandOptions["Camera Erase Heights Override"] = commandList;
      }



      ToolStripItem MakeImageSelectionButton()
      {
         ToolStripButton imageSelectButton = new ToolStripButton("Image Source:");
         imageSelectButton.Click += new EventHandler(imageSelectButton_Click);
         imageSelectButton.OwnerChanged += new EventHandler(imageSelectButton_OwnerChanged);
         return imageSelectButton;
      }

      void imageSelectButton_Click(object sender, EventArgs e)
      {
         //mTerrainEdit.
         //OpenFileDialog d = new OpenFileDialog();
         //d.Filter = "Image (*.bmp)|*.bmp";
         //if (d.ShowDialog() == DialogResult.OK)
         //{
         //   Image loadedImage = Masking.GetScaledImageFromFile(d.FileName, TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumZVerts());
        
         //   IMask mask = Masking.CreateMask(loadedImage);         
        
         //   MasklImage newImage = new MasklImage();
         //   newImage.SetMask(mask,TerrainGlobals.getTerrain().getNumXVerts(),TerrainGlobals.getTerrain().getNumZVerts());


         //   TerrainGlobals.getEditor().mCurrentAbstractImage = newImage;
         //}

         ImageSourcePicker picker = new ImageSourcePicker();
         picker.ImageSelected += new EventHandler(picker_ImageSelected);
         PopupEditor editor = new PopupEditor();
         editor.ShowPopup(this, picker);

      }

      void picker_ImageSelected(object sender, EventArgs e)
      {
         ImageSourcePicker picker = sender as ImageSourcePicker;
         TerrainGlobals.getEditor().mCurrentAbstractImage = picker.AbstractImage;
         //throw new Exception("The method or operation is not implemented.");
      }

      void imageSelectButton_OwnerChanged(object sender, EventArgs e)
      {

      }


      ScrollStripItem MakeIntensitySlider()
      {
         return MakeIntensitySlider(0.5f);
      }
      ScrollStripItem MakeIntensitySlider(float defValue)
      {
         ScrollStripItem vertBushIntensity = new ScrollStripItem("Intensity", 0.0f, 1.0f);
         vertBushIntensity.mValueChanged += new ScrollStripItem.ValueChanged(mVertBushIntensity_mValueChanged);
         vertBushIntensity.OwnerChanged += new EventHandler(vertBushIntensity_OwnerChanged);
         vertBushIntensity.Value = defValue;
         TerrainGlobals.getTerrainFrontEnd().VertBushIntensity = defValue;
         
         return vertBushIntensity;
      }
      void vertBushIntensity_OwnerChanged(object sender, EventArgs e)
      {
         try
         {
            mTerrainEdit.VertBushIntensity = ((ScrollStripItem)(sender)).Value; ;

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }        
      }

      ScrollStripItem MakeSizeSlider()
      {
         ScrollStripItem vertBrushRadius = new ScrollStripItem("Size", 0, 40);
         vertBrushRadius.mValueChanged += new ScrollStripItem.ValueChanged(mVertBrushRadius_mValueChanged);
         vertBrushRadius.OwnerChanged += new EventHandler(vertBrushRadius_OwnerChanged);
         vertBrushRadius.Value = mTerrainEdit.VertBrushRadius + 0.001f;
         
         return vertBrushRadius;
      }
      void vertBrushRadius_OwnerChanged(object sender, EventArgs e)
      {
         try
         {
            mTerrainEdit.VertBrushRadius = ((ScrollStripItem)(sender)).Value; ;

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }


      ScrollStripItem MakeHotspotSlider()
      {
         ScrollStripItem vertBrushHotspot = new ScrollStripItem("Hotspot", 0.0f, 1.0f);
         vertBrushHotspot.mValueChanged += new ScrollStripItem.ValueChanged(mVertBrushHotspot_mValueChanged);
         vertBrushHotspot.OwnerChanged += new EventHandler(vertBrushHotspot_OwnerChanged);
         vertBrushHotspot.Value = mTerrainEdit.VertBrushHotspot + 0.001f;
        
         return vertBrushHotspot;
      }
      void vertBrushHotspot_OwnerChanged(object sender, EventArgs e)
      {
         try
         {
            mTerrainEdit.VertBrushHotspot = ((ScrollStripItem)(sender)).Value;

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }


      ScrollStripItem MakeRotationSlider()
      {
         ScrollStripItem vertBrushRotation = new ScrollStripItem("Rotation", 0, 20);
         vertBrushRotation.mValueChanged += new ScrollStripItem.ValueChanged(mVertBrushRotation_mValueChanged);
         vertBrushRotation.OwnerChanged += new EventHandler(vertBrushRotation_OwnerChanged);
         vertBrushRotation.Value = 0;// mTerrainEdit.TexBrushRotation + 0.001f;

         return vertBrushRotation;
      }
      void vertBrushRotation_OwnerChanged(object sender, EventArgs e)
      {
         try
         {
            mTerrainEdit.TexBrushRotation = ((ScrollStripItem)(sender)).Value; ;

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }


      ToolStripItem MakeNoiseCheckButton()
      {
         ToolStripButton terrainNoiseButton = new ToolStripButton("Noise");
         terrainNoiseButton.Checked = false;
         terrainNoiseButton.CheckOnClick = true;
         terrainNoiseButton.CheckedChanged += new EventHandler(terrainNoiseButton_CheckedChanged);
         terrainNoiseButton.OwnerChanged += new EventHandler(terrainNoiseButton_OwnerChanged);
         return terrainNoiseButton;
      }



      void terrainNoiseButton_OwnerChanged(object sender, EventArgs e)
      {
         mTerrainEdit.NoiseEnabled = ((ToolStripButton)sender).Checked;
      }

      void terrainNoiseButton_CheckedChanged(object sender, EventArgs e)
      {
         mTerrainEdit.NoiseEnabled = ((ToolStripButton)sender).Checked;
      }



      ToolStripItem MakeSphereOrCylinderIntersectionSelectionButton()
      {
         //System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ScenarioEditorPage));

         ToolStripButton terrainSphereIntersectionButton = new ToolStripButton("Sphere");

         terrainSphereIntersectionButton.ToolTipText = "Sphere Intersection";
         terrainSphereIntersectionButton.Checked = false;
         terrainSphereIntersectionButton.CheckOnClick = true;
         terrainSphereIntersectionButton.CheckedChanged += new EventHandler(sphereOrCylinderIntersectionButton_CheckedChanged);
         terrainSphereIntersectionButton.OwnerChanged += new EventHandler(sphereOrCylinderIntersectionButton_OwnerChanged);
         terrainSphereIntersectionButton.Text = "Sphere";
         //terrainSphereIntersectionButton.DisplayStyle = ToolStripItemDisplayStyle.Image;
         //terrainSphereIntersectionButton.Image = ((System.Drawing.Image)(resources.GetObject("deformationCurve1ToolStripButton.Image")));
         return terrainSphereIntersectionButton;
      }

      void sphereOrCylinderIntersectionButton_OwnerChanged(object sender, EventArgs e)
      {
         //System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ScenarioEditorPage));

         if (!((ToolStripButton)sender).Checked)
         {
            ((ToolStripButton)sender).ToolTipText = "Sphere Intersection";
            ((ToolStripButton)sender).Text = "Sphere";
            //((ToolStripButton)sender).Image = ((System.Drawing.Image)(resources.GetObject("deformationCurve1ToolStripButton.Image")));
            mTerrainEdit.VertBrushIntersectionShape = Terrain.BrushInfo.eIntersectionShape.Sphere;
         }
         else
         {
            ((ToolStripButton)sender).ToolTipText = "Cylinder Intersection";
            ((ToolStripButton)sender).Text = "Cylinder";
            //((ToolStripButton)sender).Image = ((System.Drawing.Image)(resources.GetObject("deformationCurve2ToolStripButton.Image")));
            mTerrainEdit.VertBrushIntersectionShape = Terrain.BrushInfo.eIntersectionShape.Cylinder;
         }
      }

      void sphereOrCylinderIntersectionButton_CheckedChanged(object sender, EventArgs e)
      {
         //System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ScenarioEditorPage));

         if (!((ToolStripButton)sender).Checked)
         {
            ((ToolStripButton)sender).ToolTipText = "Sphere Intersection";
            ((ToolStripButton)sender).Text = "Sphere";
            //((ToolStripButton)sender).Image = ((System.Drawing.Image)(resources.GetObject("deformationCurve1ToolStripButton.Image")));
            mTerrainEdit.VertBrushIntersectionShape = Terrain.BrushInfo.eIntersectionShape.Sphere;
         }
         else
         {
            ((ToolStripButton)sender).ToolTipText = "Cylinder Intersection";
            ((ToolStripButton)sender).Text = "Cylinder";
            //((ToolStripButton)sender).Image = ((System.Drawing.Image)(resources.GetObject("deformationCurve2ToolStripButton.Image")));
            mTerrainEdit.VertBrushIntersectionShape = Terrain.BrushInfo.eIntersectionShape.Cylinder;
         }
      }

      ToolStripItem MakeTesselationModesCombo()
      {
         ToolStripComboBox b = new ToolStripComboBox();
        
          b.Items.Add("No Override");
          b.Items.Add("Force Min Tess");
          b.Items.Add("Force Max Tess");
        
         //Enum.Parse(typeof(ePasteMode))
         b.DropDownStyle = ComboBoxStyle.DropDownList;
         b.Width = 70;
         b.SelectedIndex = 0;
         b.SelectedIndexChanged += new EventHandler(b_SelectedTesselationIndexChanged);
         return b;
      }
      void b_SelectedTesselationIndexChanged(object sender, EventArgs e)
      {
         ToolStripComboBox b = sender as ToolStripComboBox;
         mTerrainEdit.TesselationValue = (BTerrainEditor.eTessOverrideVal)b.SelectedIndex;
      }
      ToolStripItem makeTessellationClearButton()
      {
         ToolStripButton tessSelectButton = new ToolStripButton("Clear Tessellation");
         tessSelectButton.Click += new EventHandler(tessSelectButton_Click);
         
         //imageSelectButton.OwnerChanged += new EventHandler(imageSelectButton_OwnerChanged);
         return tessSelectButton;
      }
      void tessSelectButton_Click(object sender, EventArgs e)
      {
         if (MessageBox.Show("Are you sure you'd like to clear?", "Please Confirm", MessageBoxButtons.OKCancel) != DialogResult.OK)
            return;
         TerrainGlobals.getEditor().clearTesselationOverride();
      }




      ToolStripItem MakePassableModesCombo()
      {
         ToolStripComboBox b = new ToolStripComboBox();

         b.Items.Add("No Override");
         b.Items.Add("Force Passable");
         b.Items.Add("Force Unpassable");

         //Enum.Parse(typeof(ePasteMode))
         b.DropDownStyle = ComboBoxStyle.DropDownList;
         b.Width = 70;
         b.SelectedIndex = 2;
         b.SelectedIndexChanged += new EventHandler(b_SelectedPassableIndexChanged);
         return b;
      }
      void b_SelectedPassableIndexChanged(object sender, EventArgs e)
      {
         ToolStripComboBox b = sender as ToolStripComboBox;

         if (b.SelectedIndex == 0)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setPassableBrushState(SimTileData.ePassOverrideVal.cPass_None);
         else if (b.SelectedIndex == 1)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setPassableBrushState(SimTileData.ePassOverrideVal.cPass_Passable);
         else if (b.SelectedIndex == 2)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setPassableBrushState(SimTileData.ePassOverrideVal.cPass_Unpassable);
      }
      ToolStripItem makePassableClearButton()
      {
         ToolStripButton tessSelectButton = new ToolStripButton("Clear Passability");
         tessSelectButton.Click += new EventHandler(passSelectButton_Click);

         return tessSelectButton;
      }
      void passSelectButton_Click(object sender, EventArgs e)
      {
         if (MessageBox.Show("Are you sure you'd like to clear?", "Please Confirm", MessageBoxButtons.OKCancel) != DialogResult.OK)
            return;

         TerrainGlobals.getEditor().getSimRep().getDataTiles().clearPassableOverride();
         TerrainGlobals.getEditor().getSimRep().update();// updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
      }



      ToolStripItem MakeBuildableModesCombo()
      {
         ToolStripComboBox b = new ToolStripComboBox();

         b.Items.Add("No Override");
         b.Items.Add("Force Buildable");
         b.Items.Add("Force Unbuildable");

         //Enum.Parse(typeof(ePasteMode))
         b.DropDownStyle = ComboBoxStyle.DropDownList;
         b.Width = 70;
         b.SelectedIndex = 2;
         b.SelectedIndexChanged += new EventHandler(b_SelectedBuildableIndexChanged);
         return b;
      }
      void b_SelectedBuildableIndexChanged(object sender, EventArgs e)
      {
         ToolStripComboBox b = sender as ToolStripComboBox;
         if (b.SelectedIndex == 0)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setBuildibleBrushState(SimTileData.eBuildOverrideVal.cBuild_None);
         else if (b.SelectedIndex == 1)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setBuildibleBrushState(SimTileData.eBuildOverrideVal.cBuild_Buildable);
         else if (b.SelectedIndex == 2)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setBuildibleBrushState(SimTileData.eBuildOverrideVal.cBuild_Unbuildable);
      }
      ToolStripItem makeBuildableClearButton()
      {
         ToolStripButton tessSelectButton = new ToolStripButton("Clear Buildable");
         tessSelectButton.Click += new EventHandler(buildSelectButton_Click);

         return tessSelectButton;
      }
      void buildSelectButton_Click(object sender, EventArgs e)
      {
         if (MessageBox.Show("Are you sure you'd like to clear?", "Please Confirm", MessageBoxButtons.OKCancel) != DialogResult.OK)
            return;

         TerrainGlobals.getEditor().getSimRep().getDataTiles().clearBuildableOverride();
         TerrainGlobals.getEditor().getSimRep().update();// updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), true);
      }






      ToolStripItem MakeFloodPassableModesCombo()
      {
         ToolStripComboBox b = new ToolStripComboBox();

         b.Items.Add("No Override");
         b.Items.Add("Force Passable");
         b.Items.Add("Force Unpassable");

         //Enum.Parse(typeof(ePasteMode))
         b.DropDownStyle = ComboBoxStyle.DropDownList;
         b.Width = 70;
         b.SelectedIndex = 2;
         b.SelectedIndexChanged += new EventHandler(b_SelectedFloodPassableIndexChanged);
         return b;
      }
      void b_SelectedFloodPassableIndexChanged(object sender, EventArgs e)
      {
         ToolStripComboBox b = sender as ToolStripComboBox;
         if (b.SelectedIndex == 0)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setFloodPassableBrushState(SimTileData.eFloodPassOverrideVal.cFldPss_None);
         else if (b.SelectedIndex == 1)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setFloodPassableBrushState(SimTileData.eFloodPassOverrideVal.cFldPss_FloodPassable);
         else if (b.SelectedIndex == 2)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setFloodPassableBrushState(SimTileData.eFloodPassOverrideVal.cFldPss_UnFloodPassable);
      }
      ToolStripItem makeFloodPassableClearButton()
      {
         ToolStripButton tessSelectButton = new ToolStripButton("Clear FloodPassable");
         tessSelectButton.Click += new EventHandler(FloodPassableSelectButton_Click);

         return tessSelectButton;
      }
      void FloodPassableSelectButton_Click(object sender, EventArgs e)
      {
         if (MessageBox.Show("Are you sure you'd like to clear?", "Please Confirm", MessageBoxButtons.OKCancel) != DialogResult.OK)
            return;

         TerrainGlobals.getEditor().getSimRep().getDataTiles().clearFloodPassableOverride();
         TerrainGlobals.getEditor().getSimRep().update();
      }





      ToolStripItem MakeScarabPassableModesCombo()
      {
         ToolStripComboBox b = new ToolStripComboBox();

         b.Items.Add("No Override");
         b.Items.Add("Force Passable");
         b.Items.Add("Force Unpassable");

         //Enum.Parse(typeof(ePasteMode))
         b.DropDownStyle = ComboBoxStyle.DropDownList;
         b.Width = 70;
         b.SelectedIndex = 2;
         b.SelectedIndexChanged += new EventHandler(b_SelectedScarabPassableIndexChanged);
         return b;
      }
      void b_SelectedScarabPassableIndexChanged(object sender, EventArgs e)
      {
         ToolStripComboBox b = sender as ToolStripComboBox;
         if (b.SelectedIndex == 0)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setScarabPassableBrushState(SimTileData.eScarabPassOverrideVal.cScrbPss_None);
         else if (b.SelectedIndex == 1)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setScarabPassableBrushState(SimTileData.eScarabPassOverrideVal.cScrbPss_ScarabPassable);
         else if (b.SelectedIndex == 2)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setScarabPassableBrushState(SimTileData.eScarabPassOverrideVal.cScrbPss_UnScarabPassable);
      }
      ToolStripItem makeScarabPassableClearButton()
      {
         ToolStripButton tessSelectButton = new ToolStripButton("Clear ScarabPassable");
         tessSelectButton.Click += new EventHandler(ScarabPassableSelectButton_Click);

         return tessSelectButton;
      }
      void ScarabPassableSelectButton_Click(object sender, EventArgs e)
      {
         if (MessageBox.Show("Are you sure you'd like to clear?", "Please Confirm", MessageBoxButtons.OKCancel) != DialogResult.OK)
            return;

         TerrainGlobals.getEditor().getSimRep().getDataTiles().clearScarabPassableOverride();
         TerrainGlobals.getEditor().getSimRep().update();
      }



      ToolStripItem MakeTileTypeModesCombo()
      {
         ToolStripComboBox b = new ToolStripComboBox();

         b.Items.Add("No Override");
         b.Items.Add("Override");

         b.DropDownStyle = ComboBoxStyle.DropDownList;
         b.Width = 90;
         b.SelectedIndex = 0;
         b.SelectedIndexChanged += new EventHandler(b_SelectedTileTypeModeIndexChanged);
         return b;
      }
      void b_SelectedTileTypeModeIndexChanged(object sender, EventArgs e)
      {
         ToolStripComboBox b = sender as ToolStripComboBox;
         if (b.SelectedIndex == 0)
         {
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setTileTypeBrushState(SimTileData.eTileTypeOverrideVal.cTileType_None);
            CurrentCommandOptionsToolStrip.Items["TileTypesCombo"].Enabled = false;
         }
         else
         {
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setTileTypeBrushState(SimTileData.eTileTypeOverrideVal.cTileType_Override);
            CurrentCommandOptionsToolStrip.Items["TileTypesCombo"].Enabled = true;
         }
      }

      ToolStripItem MakeTileTypesCombo()
      {
         ToolStripComboBox b = new ToolStripComboBox();

         SimTerrainType.loadTerrainTileTypes();

         for (int i = 0; i < SimTerrainType.getNumTileTypes(); i++)
         {
            b.Items.Add(SimTerrainType.getTileTypeByIndex(i).Name);
         }
   
         b.Name = "TileTypesCombo";
         b.DropDownStyle = ComboBoxStyle.DropDownList;
         b.Width = 90;
         b.SelectedIndex = 0;
         b.SelectedIndexChanged += new EventHandler(b_SelectedTileTypesIndexChanged);
         b.Enabled = false;
         return b;
      }
      void b_SelectedTileTypesIndexChanged(object sender, EventArgs e)
      {
         ToolStripComboBox b = sender as ToolStripComboBox;
         if (b.SelectedIndex != -1)
            TerrainGlobals.getEditor().getSimRep().getDataTiles().setTileTypeOverrideSelection(SimTerrainType.getTileTypeByIndex(b.SelectedIndex).Name);
         
      }

      ToolStripItem makeTileTypeClearButton()
      {
         ToolStripButton tessSelectButton = new ToolStripButton("Clear Tile Type");
         tessSelectButton.Click += new EventHandler(TileTypeSelectButton_Click);

         return tessSelectButton;
      }
      void TileTypeSelectButton_Click(object sender, EventArgs e)
      {
         if (MessageBox.Show("Are you sure you'd like to clear?", "Please Confirm", MessageBoxButtons.OKCancel) != DialogResult.OK)
            return;

         TerrainGlobals.getEditor().getSimRep().getDataTiles().clearTileTypeOverride();
         TerrainGlobals.getEditor().getSimRep().update();
      }



      ToolStripItem MakeSmudgeBrushCombo()
      {
         ToolStripComboBox b = new ToolStripComboBox();

         b.Items.Add("Y Axis");
         b.Items.Add("Vert Normal");

         //Enum.Parse(typeof(ePasteMode))
         b.DropDownStyle = ComboBoxStyle.DropDownList;
         b.Width = 70;
         b.SelectedIndex = 0;
         b.SelectedIndexChanged += new EventHandler(b_SelectedSmudgeBrushIndexChanged);
         return b;
      }
      void b_SelectedSmudgeBrushIndexChanged(object sender, EventArgs e)
      {
         ToolStripComboBox b = sender as ToolStripComboBox;
         mTerrainEdit.SmudgeBrushUseNormal = b.SelectedIndex == 0 ? false : true;
      }



      ToolStripItem MakeScaleBrushModesCombo()
      {
         ToolStripComboBox b = new ToolStripComboBox();

         b.Items.Add("XYZ");
         b.Items.Add("XZ Only");

         //Enum.Parse(typeof(ePasteMode))
         b.DropDownStyle = ComboBoxStyle.DropDownList;
         b.Width = 70;
         b.SelectedIndex = 0;
         b.SelectedIndexChanged += new EventHandler(b_SelectedScaleBrushIndexChanged);
         return b;
      }
      void b_SelectedScaleBrushIndexChanged(object sender, EventArgs e)
      {
         ToolStripComboBox b = sender as ToolStripComboBox;
         mTerrainEdit.ScalarBrushXZOnly = b.SelectedIndex==0?false:true;
      }

     


      ToolStripItem MakeRotateClipModesCombo()
      {
         ToolStripComboBox b = new ToolStripComboBox();
         string[] pasteops = Enum.GetNames(typeof(BTerrainFrontend.ePasteOperation));
         foreach(string s in pasteops)
         {
            b.Items.Add(s);
         }
         //Enum.Parse(typeof(ePasteMode))
         b.DropDownStyle = ComboBoxStyle.DropDownList;
         b.Width = 70;
         b.SelectedIndex = (int)mTerrainEdit.PasteOperation;
         b.SelectedIndexChanged += new EventHandler(b_SelectedIndexChanged);
         return b;
      }

      void b_SelectedIndexChanged(object sender, EventArgs e)
      {
         ToolStripComboBox b = sender as ToolStripComboBox;
         mTerrainEdit.PasteOperation = (BTerrainFrontend.ePasteOperation)b.SelectedIndex;
      }
      ToolStripItem MakeOKPaste()
      {
         ToolStripButton okPasteButton = new ToolStripButton("OK");       
         okPasteButton.Click += new EventHandler(okPasteButton_Click);
         //okPasteButton.OwnerChanged += new EventHandler(clipartRotateCW90_OwnerChanged);
         return okPasteButton;
      }

      void okPasteButton_Click(object sender, EventArgs e)
      {
         mTerrainEdit.OKPaste();
      }
      ToolStripItem MakeCancelPaste()
      {
         ToolStripButton cancelPasteButton = new ToolStripButton("Cancel");
         cancelPasteButton.Click += new EventHandler(cancelPasteButton_Click);
         //okPasteButton.OwnerChanged += new EventHandler(clipartRotateCW90_OwnerChanged);
         return cancelPasteButton;
      }

      void cancelPasteButton_Click(object sender, EventArgs e)
      {
         mTerrainEdit.CancelPaste();
      }


      public void ToolbarButtonClicked(UiGenerator.SomeCommand command)
      {
         HighlightActiveCommand(command.GetName());

         //SetToolBarUI(command.GetName());
      }

      public void HighlightActiveCommand(string commandName)
      {
         foreach (ToolStripButton b in SimToolStrip.Items)
         {
            b.Checked = false;
            if (b.ToolTipText == commandName)
            {
               b.Checked = true;
            }
         }



      }

      public void HandleCommand(string commandName)
      {
         SetToolBarUI(commandName);

         HighlightActiveCommand(commandName);
      }

    
      private void SetToolBarUI(string name)
      {
         CurrentCommandOptionsToolStrip.SuspendLayout();
         CurrentCommandOptionsToolStrip.MinimumSize = CurrentCommandOptionsToolStrip.Size;
         CurrentCommandOptionsToolStrip.Items.Clear();


         mTerrainEdit.NoiseEnabled = false;
         mTerrainEdit.VertBrushIntersectionShape = Terrain.BrushInfo.eIntersectionShape.Sphere;

         if(mToolstripCommandOptions.ContainsKey(name))
         {
            CurrentCommandOptionsToolStrip.Items.AddRange(mToolstripCommandOptions[name].ToArray());

            foreach (ToolStripItem t in mToolstripCommandOptions[name])
            {
               if (t is ScrollStripItem)
               {
                  ScrollStripItem s = t as ScrollStripItem;
                  s.Value = s.Value;
                  
               }
               else if(t is ToolStripButton)
               {
                  ToolStripButton b = t as ToolStripButton;
                  if(b.CheckOnClick == true)
                  {
                     b.Checked = b.Checked;
                     
                  }
               }


            }
         }

         CurrentCommandOptionsToolStrip.ResumeLayout();

      }




      void mVertBrushRadius_mValueChanged(float val)
      {
         try
         {
            mTerrainEdit.VertBrushRadius = val;

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      void mVertBrushHotspot_mValueChanged(float val)
      {
         try
         {
            mTerrainEdit.VertBrushHotspot = val;

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      void mVertBushIntensity_mValueChanged(float val)
      {
         try
         {
            mTerrainEdit.VertBushIntensity = val;

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      void mVertBrushRotation_mValueChanged(float val)
      {
         try
         {
            mTerrainEdit.TexBrushRotation = val;

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      #endregion 


      



      public bool mDoingLoad = false;

      private void NewClipArt()
      {
         NewClipArt c = new NewClipArt();
         //if (data.HasData() == false)
         {
            TerrainGlobals.getEditor().CopySelected();
         }
         ClipArtData data = TerrainGlobals.getEditor().mClipArtData;
         if (data.HasData() == false)
         {
            MessageBox.Show("You must first select a region to copy (hold control and paint the selection)");
         }
         else
         {
            data.SetThumbnail(TerrainGlobals.getTerrainFrontEnd().renderThumbnail(true,true));
            c.SetClipArtData(data);
            c.ShowDialog();
         }
         SimGlobals.getSimMain().clipartClearObjects();
      }

      int mLastWidth = -1;//int.MaxValue;
      int mLastHeight = -1;//int.MaxValue;

      //CLM [04.26.06] this function called when a tab becomes 'active'
      public override void Activate()
      {
         mbSuppresDeviceAutoSave = true;

         //mbInited = true;
         base.Activate();
         BRenderDevice.setZNearPlane(5.0f);
         BRenderDevice.setZFarPlane(10000.0f);


         //if (!PreventDeviceResize && (mLastWidth < GetSubTarget().Width || mLastHeight < GetSubTarget().Height))
         if (!PreventDeviceResize && (mLastWidth != GetSubTarget().Width || mLastHeight != GetSubTarget().Height))
         {
            //GetSubTarget().Left
            //this call passes in the panel width and height
            CoreGlobals.getEditorMain().mIGUI.deviceResize(GetSubTarget().Width, GetSubTarget().Height, false);

            mLastHeight = GetSubTarget().Height;
            mLastWidth = GetSubTarget().Width;

            //CLM [07.31.07] UGGGGG
            FoliageManager.clearAllChunkVBs();

         }
         //this call passes in the panel handle in which you want to render to.
         //this is handled during the 'present' call
         BRenderDevice.setWindowTarget(GetSubTarget());

         mbSuppresDeviceAutoSave = false;
      }

      public override Control GetUITarget()
      {
         return GetSubTarget();
      }

      private Control GetSubTarget()
      {
         if(this.TabControl.SelectedTab != null && this.TabControl.SelectedTab.Tag != null)
         {
            IGraphicsWindowOwner gwnd = this.TabControl.SelectedTab.Tag as IGraphicsWindowOwner;
            if(gwnd != null)
            {
               return gwnd.GetGraphicsWindow();

            }
         }
         return this.GetGraphicsWindow();
      }


      //CLM [04.26.06] this function called when a tab becomes 'deactive'
      public override void Deactivate()
      {

      }

      //CLM [04.26.06] called when this page is closed from the main tab
      public override void destroyPage()
      {
         base.destroyPage();
         deinitDeviceData();
         deinit();
      }


      //these two functions are called when the tab is created, and deinitalized respectively
      public override void init()
      {
         using (PerfSection p = new PerfSection("SecenarioEditorPage.init()"))
         {

            DDXBridge.init();
            SimTerrainType.loadTerrainTypes();

            mTerrainEdit = TerrainGlobals.getTerrainFrontEnd();
            mTerrainEdit.init();

            mSimMain = SimGlobals.getSimMain();

            // Set default brush to Std (standard)
            mTerrainEdit.SelectedMaskIndex = 0;
            mTerrainEdit.BrushesStdBrush();

            InitToolBars();

            //this.mScenarioSourceControl.PerforceStatusChangedEvent += new ScenarioSourceControl.PerforceStatusChanged(scenarioSourceControl1_PerforceStatusChangedEvent);
            //this.mScenarioSourceControl.MissingFiles += new ScenarioSourceControl.FileEvent(scenarioSourceControl1_MissingFiles);


            mSimMain.Changed += new SimMain.SimChanged(mSimMain_Changed);
            mSimMain.LightsChanged += new SimMain.SimChanged(mSimMain_LightsChanged);


            // Initialize granny manager
            GrannyManager2.init();


            //
            mlScenarioScriptsManager.SimMain = mSimMain;

            InitDepartmentCombo();
         }
      }

      void scenarioSourceControl1_MissingFiles(List<string> fileNames)
      {
         //if(MessageBox.Show("You must export the scenario first.  Do this now?","Missing files", MessageBoxButtons.YesNo) == DialogResult.Yes )
         //{
         //   DoExport();
         //   this.mScenarioSourceControl.SubmitFiles();
         //}
      }

      void scenarioSourceControl1_PerforceStatusChangedEvent(string filename)
      {
         UpdatePerforceIcon(filename);
      }

      public override void deinit()
      {
         base.deinit();
         DDXBridge.destroy();
         mTerrainEdit.destroy();
         stopQuickView();
      }

      //CLM [04.26.06] these functions called for all data that's not in the MANAGED pool for d3d.
      //on a device resize, or reset, these functions are called for you.
      //bool mbInited = false;
      
      public override void initDeviceData()
      {
         using (PerfSection p = new PerfSection("initDeviceData"))
         {
            base.initDeviceData();
            mTerrainEdit.initDeviceData();

            if (mbAutosavedFromCtrlDel)
            {
               CoreGlobals.ShowMessage("Device was lost:  Scenario autosaved to _autoSave directory");
               mbAutosavedFromCtrlDel = false;
            }

         }
      }

      bool mbAutosavedFromCtrlDel = false;
      bool mbSuppresDeviceAutoSave = false;
      DateTime mLastDeviceLostTime = System.DateTime.Now;
      override public void deinitDeviceData()
      {
         //if (mbInited == true)
         {
            //mbInited = false;
            base.deinitDeviceData();
            mTerrainEdit.deinitDeviceData();


            //WTF this does not work!!!!!
            //lock (this)
            //{
            //   double delay = ((TimeSpan)(System.DateTime.Now - mLastDeviceLostTime)).TotalSeconds;
            //   if (delay > 5)
            //   {
            //      if (mbSuppresDeviceAutoSave == false)
            //      {
            //         doQuickSave("_autoSave");
            //         mbAutosavedFromCtrlDel = true;
            //         mLastDeviceLostTime = System.DateTime.Now;
            //         Console.Out.WriteLine("Autosave delayvalue:" + delay.ToString() + "  at  "+ System.DateTime.Now.ToString());
            //      }
            //   }
            //   else
            //   {
            //      Console.Out.WriteLine("not ready yet..");
            //   }
            //}
         }
      }
      

      //override these functions to ensure your app gets the proper processing.
      public override void alwaysProcess()
      {
         SimGlobals.getSimMain().CheckForChanges();
      }
      
      override public void input() 
      {
         if (DoRealTimeStuff())
         {
            base.input();
            mTerrainEdit.input();
         }
      }
      override public void update()
      {
         if (DoRealTimeStuff())
         {
            base.update();
            mTerrainEdit.update();
            flushWaitingEditorMessages();
         }

         GrannyManager2.reloadChangedResources();
      }
      override public void render()
      {
         if (DoRealTimeStuff())
         {
            base.render();
            mTerrainEdit.render();
         }
      }

      bool DoRealTimeStuff()
      {

         if (this.TabControl.SelectedTab != null && this.TabControl.SelectedTab.Tag != null)
         {
            IGraphicsWindowOwner gwnd = this.TabControl.SelectedTab.Tag as IGraphicsWindowOwner;
            if (gwnd != null)
            {
               if ((gwnd.GetGraphicsWindow() != null) && (gwnd.GraphicsEnabled() == true))
               {
                  return true;
               }
            }

         }
         return false;

      }

      void flushWaitingEditorMessages()
      {
         lock(CoreGlobals.getEditorMain().mMessageList)
         {
            int ic = CoreGlobals.getEditorMain().mMessageList.Count();
            for(int i=0;i<ic;i++)
            {
               CoreGlobals.getEditorMain().mMessageList.dequeueProcessMessage();
            }
         }
      }
      #region Save Load Code


      //these functions will be called from the file menu. It's the panel's job to call the proper dialog
      override public void save_file()
      {
         {
            if (CoreGlobals.ScenarioFile.CompareTo("") == 0)
            {
               save_file_as();

               return;
            }
            else
            {
               string fileName = Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioFile);
               SaveProject(fileName);
            }
         }
      }
      override public void save_file_as()
      {
         SaveProject sp = new SaveProject();

         if (sp.ShowDialog() == DialogResult.OK)
         {
            SaveProject(sp.FileName);
         }
      }
      override public void open_file(string path)
      {
         LoadProject(path);
      }
      override public void open_file()
      {
         OpenProject d = new OpenProject();

         if (d.ShowDialog() == DialogResult.OK)
         {
            open_file(d.FileName);
         }
      }
      public void NewScenarioTopics()
      {
         mScenarioSourceControl2.Reset();

         Dictionary<string, WorkTopic> workTopics = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics();
         workTopics.Clear();
         workTopics["Terrain"] = new WorkTopic("Terrain", "Terrain, Sim Rep, Camera Rep", ".TED", new string[] { });
         workTopics["Lights"] = new WorkTopic("Lights", "Global Lights, FLS, Spots and Omnis", ".FLS, .FLS.XMB, .GLS, .GLS.XMB", new string[] { });
         workTopics["Sim"] = new WorkTopic("Sim", "Designer Objects, Scripts, Scenario Settings", ".SCN, .SCN.XMB", new string[] { });
         workTopics["ArtObjects"] = new WorkTopic("ArtObjects", "Art Objects", ".SC2, .SC2.XMB", new string[] { });
         workTopics["Sounds"] = new WorkTopic("Sounds", "Sound Objects", ".SC3, .SC3.XMB", new string[] { });
         workTopics["Masks"] = new WorkTopic("Masks", "Masks", "(editorData.zip)", new string[] { });

         workTopics["Export"] = new WorkTopic("Export", "Game files that get exported", ".XSD, .XTD, .XTH, .XTT, .LRP, .DEP, .TAG ", new string[] { });
         workTopics["Export"].mDataPattern = WorkTopic.eDataPatterm.cSecondary;
      }
      //public void OpenScenarioTopics(string filename)
      //{
      //   Dictionary<string, WorkTopic> workTopics = CoreGlobals.getScenarioWorkTopics();
      //   workTopics.Clear();
      //   workTopics["Terrain"] = new WorkTopic("Terrain", "Terrain and sim rep", new string[] { });
      //   workTopics["Sim"] = new WorkTopic("Sim", "Objects, Scripts, Scenario Settings", new string[] { });
      //   workTopics["Lights"] = new WorkTopic("Lights", "Global Lights, Spots and Omnis", new string[] { });
      //   workTopics["Masks"] = new WorkTopic("Masks", "Masks", new string[] { });
      //}

      public void NewProject(TerrainCreationParams param)
      {
         using (PerfSection p2 = new PerfSection("NewProject()"))
         {
            NewScenarioTopics();
            mTerrainEdit.NewTerrain(param);

            //export?
            //ResetP4Icon();
            //if (CoreGlobals.UsingPerforce)
            //{
            //   this.mScenarioSourceControl.Enabled = false;
            //   this.mScenarioSourceControl.SetFileList(new List<string>());
            //}

            LoadScenarioPlayerData();
            LoadObjectivesData();
            LoadTriggerData();
            this.mTempLightSettings.reload();

            mbHasClickedQuickView = false;

            ResetCamera();

            timer1.Enabled = CoreGlobals.getSettingsFile().AutoSaveEnabled;
            timer1.Interval = CoreGlobals.getSettingsFile().AutoSaveTimeInMinutes * 60000;
            timer2.Interval = 600000;
         }
      }




      bool mbHasClickedQuickView = false;
      void mSimMain_Changed(SimMain sender)
      {
         if (mbHasClickedQuickView == false || xBoxAutoUpdateObjectsToolStripMenuItem.Checked == false) return;

         string scenerioPath = CoreGlobals.getWorkPaths().mGameScenarioDirectory;
         if (!Directory.Exists(scenerioPath + @"\quickview"))
            Directory.CreateDirectory(scenerioPath + @"\quickview");
         //mTerrainEdit.SaveProject(scenerioPath + @"\quickview\quickview.scn", false, false);
         string scenarioName = scenerioPath + @"\quickview\quickview.scn";
         string terrainName = Path.ChangeExtension(scenarioName, "ted");
         string lightsetName = Path.ChangeExtension(scenarioName, "gls");

         bool bNoScript = noScriptToolStripMenuItem.Checked;

         SimGlobals.getSimMain().SaveScenario(scenarioName, terrainName, lightsetName, bNoScript);
         SimGlobals.getSimMain().SaveExtasMacro(scenarioName);

         WorkTopic topic;
         if (CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics().TryGetValue("Sim", out topic))
         {
            topic.Changed = true;
         }

         //CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics()["Sim"].Changed = true;
      }

      void mSimMain_LightsChanged(SimMain sender)
      {
         //throw new Exception("The method or operation is not implemented.");

         if (mbHasClickedQuickView == false || xBoxAutoUpdateObjectsToolStripMenuItem.Checked == false) return;

         string scenerioPath = CoreGlobals.getWorkPaths().mGameScenarioDirectory;
         if (!Directory.Exists(scenerioPath + @"\quickview"))
            Directory.CreateDirectory(scenerioPath + @"\quickview");
         
         this.mTempLightSettings.saveXML(scenerioPath + @"\quickview\quickview.gls");

         WorkTopic topic;
         if (CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics().TryGetValue("Lights", out topic))
         {
            topic.Changed = true;
         }
         //CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics()["Lights"].Changed = true;
      }

      public void BakeTriggerData()
      {
         //mSimMain.TriggerData = this.mFirstTriggerEditor.TriggerData;  //this is a ref but getting the property does the final baking

         List<TriggerRoot> systems = new List<TriggerRoot>();

         foreach(TabPage p in TabControl.Controls)
         {
            TriggerEditor ed = p.Tag as TriggerEditor;

            if(ed != null)
            {
               systems.Add(ed.TriggerData); //this is a ref but getting the property does the final baking
            }
         }
         //external script bake.   this is off for phoenix
         //foreach(ExternalScriptInfo ext in mSimMain.ExternalScripts)
         //{
         //   bool error = false;
         //   try
         //   {
         //      string filename = Path.Combine(CoreGlobals.getWorkPaths().mScriptTriggerDirectory, ext.FileName);
         //      if (File.Exists(filename))
         //      {
         //         XmlSerializer s = new XmlSerializer(typeof(TriggerRoot), new Type[] { });
         //         Stream st = File.OpenRead(filename);
         //         TriggerRoot root = (TriggerRoot)s.Deserialize(st);

         //         root.External = true;
         //         st.Close();

         //         systems.Add(root);
         //      }
         //      else
         //      {
         //         error = true;
         //      }
         //   }
         //   catch (System.Exception ex)
         //   {
         //      CoreGlobals.getErrorManager().OnException(ex, true);
         //      error = true;
         //   }
         //   if(error)
         //   {
         //      MessageBox.Show("Error with external script: " + ext.FileName);
         //   }
         //}


         mSimMain.TriggerData = systems;

      }
      public bool PreExportEvaluate()
      {
         Dictionary<string, WorkTopic> workTopics = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics();

         Dictionary<string, WorkTopic> topics = new Dictionary<string, WorkTopic>();
         topics["Export"] = workTopics["Export"];

         topics["Export"].DoNotSave = false; //we need to override this... it is not valid

         return CheckForBlockedTopics(topics);

      }

      public void SetTopicFiles()
      {

         Dictionary<string, WorkTopic> topics = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics();
         topics["Terrain"].Files.Clear();
         topics["Terrain"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.TerrainFile));


         topics["Sim"].Files.Clear();
         topics["Sim"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioFile));
         topics["Sim"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioFile + ".xmb"));
         topics["Sim"].EnableFileWatcher();


         topics["ArtObjects"].Files.Clear();
         topics["ArtObjects"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioArtFile));
         topics["ArtObjects"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioArtFile + ".xmb"));

         topics["Sounds"].Files.Clear();
         topics["Sounds"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioSoundFile));
         topics["Sounds"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioSoundFile + ".xmb"));

         topics["Masks"].Files.Clear();
         topics["Masks"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioEditorSettingsFilename));

         topics["Lights"].Files.Clear();
         topics["Lights"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioLightsetFilename));
         topics["Lights"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioLightsetFilename + ".xmb"));
         topics["Lights"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, Path.ChangeExtension(CoreGlobals.ScenarioLightsetFilename, "fls")));
         topics["Lights"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, Path.ChangeExtension(CoreGlobals.ScenarioLightsetFilename, "fls") + ".xmb"));
         //Not optional files must be listed under files, and optional files.
         topics["Lights"].OptionalFiles.Clear();
         topics["Lights"].OptionalFiles.Add(Path.Combine(CoreGlobals.ScenarioDirectory, Path.ChangeExtension(CoreGlobals.ScenarioLightsetFilename, "fls")));
         topics["Lights"].OptionalFiles.Add(Path.Combine(CoreGlobals.ScenarioDirectory, Path.ChangeExtension(CoreGlobals.ScenarioLightsetFilename, "fls") + ".xmb"));

         //Put these in a separate bucket?
         //Export files:
         topics["Export"].Files.Clear();
         topics["Export"].Files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "xtd")));
         topics["Export"].Files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "xtt")));
         topics["Export"].Files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "xth")));
         topics["Export"].Files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.ScenarioFile, "xsd")));
         topics["Export"].Files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.ScenarioFile, "lrp")));
         topics["Export"].Files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "dep")));
         //topics["Export"].Files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "dep.xmb")));
         topics["Export"].Files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "tag")));
      }

      public bool PreSaveEvaluate()
      {
         Dictionary<string, WorkTopic> workTopics = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics();
         Dictionary<string, WorkTopic> topics = new Dictionary<string, WorkTopic>();
         topics["Terrain"] = workTopics["Terrain"];
         topics["Sim"] = workTopics["Sim"];
         topics["ArtObjects"] = workTopics["ArtObjects"];
         topics["Sounds"] = workTopics["Sounds"];
         topics["Masks"] = workTopics["Masks"];
         topics["Lights"] = workTopics["Lights"];

         return CheckForBlockedTopics(topics);
      }

      private void ClearDoNotSaveFlags()
      {
         Dictionary<string, WorkTopic>.Enumerator it = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics().GetEnumerator();
         while (it.MoveNext())
         {
            it.Current.Value.DoNotSave = false;
         }
      }

      private static bool CheckForBlockedTopics(Dictionary<string, WorkTopic> topics)
      {
         bool filesBlocked = false;
         Dictionary<string, WorkTopic>.Enumerator it = topics.GetEnumerator();
         while (it.MoveNext())
         {
            it.Current.Value.UpdateState();
            if (it.Current.Value.Changed == true)
            {
               ICollection<string> blocked;

               //if (it.Current.Value.mDataPattern != WorkTopic.eDataPatterm.cPrimary)
               //   continue;

               if (CoreGlobals.getScenarioWorkTopicManager().sDontPromtResolvedSave && it.Current.Value.DoNotSave)
                  continue;

               if (it.Current.Value.AreFilesWritetable(out blocked) == false)
               {
                  //CoreGlobals.ShowMessage(it.Current.Value + " not writeable...");
                  filesBlocked = true;
               }
               else
               {

               }

            }
            else
            {

            }
         }

         return filesBlocked;
      }

      string mLastName = "";


      string backupScenarioDirectory;
      string backupTerrainDirectory;
      string backupTerrainFile;
      string backupScenarioFile;
      string backupScenarioLightsetFilename;
      private void BackupGlobalFileNames()
      {
         backupScenarioDirectory = CoreGlobals.ScenarioDirectory ;
         backupTerrainDirectory = CoreGlobals.TerrainDirectory;
         backupTerrainFile = CoreGlobals.TerrainFile;
         backupScenarioFile = CoreGlobals.ScenarioFile;
         backupScenarioLightsetFilename = CoreGlobals.ScenarioLightsetFilename;         
      }
      private void RestoreGlobalFileNames()
      {
         CoreGlobals.ScenarioDirectory = backupScenarioDirectory;
         CoreGlobals.TerrainDirectory = backupTerrainDirectory;
         CoreGlobals.TerrainFile = backupTerrainFile;
         CoreGlobals.ScenarioFile = backupScenarioFile;
         CoreGlobals.ScenarioLightsetFilename = backupScenarioLightsetFilename;         
      }

      public bool CheckTopicPermission(string topicName)
      {


         if (topicName == "" || topicName == "Deferred")
         {
            return true;
         }
         WorkTopic topic;
         if (CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics().TryGetValue(topicName, out topic))
         {
            if (topic.Paused == true)
            {
               return true;
            }
            if (topic.mState == WorkTopic.eWorkTopicFileState.cStateNotSet)
            {
               topic.UpdateState();
               //return true;
            }
            if (topic.DoNotSave == true)
            {
               return true;
            }
            if (topic.mState == WorkTopic.eWorkTopicFileState.cNoFiles)
            {
               return true;
            }
            if (topic.mState == WorkTopic.eWorkTopicFileState.cLocalWriteable)
            {   
               //Warning
               return true;
            }
            else if (topic.mState == WorkTopic.eWorkTopicFileState.cCheckedOutByUser)
            {
               //Warning
               return true;
            }

            if (topic.Writeable == true)
            {               
               return true;
            }

            //then.. update state
            topic.UpdateState();


            PopupEditor pe = new PopupEditor();
            SingleTopicCoordinator swc = new SingleTopicCoordinator();
            swc.SetTopic(topicName);
            if (pe.ShowPopup(this, swc, FormBorderStyle.FixedDialog, true, "Edit " + topicName + "?").ShowDialog() != DialogResult.OK)
            {
               //Revert FileNames refresh perforce settings
               //RestoreGlobalFileNames();
               //SetTopicFiles();
               mScenarioSourceControl2.Reset();
               return false;
            }

            mScenarioSourceControl2.UpdateUI();

            return true;
         }

         return true;

      }



      public void SaveProject(string fileName)
      {
         Dictionary<string, WorkTopic> workTopics = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics();


         string scenariofilename = fileName;
         string directory = Path.GetDirectoryName(scenariofilename);
         string lightsetfile = Path.ChangeExtension(scenariofilename, "gls");
         string terrainFile = Path.ChangeExtension(scenariofilename, "TED");

         bool savingAsDifferent = false;
         if (CoreGlobals.ScenarioFile != Path.GetFileName(scenariofilename))
         {
            savingAsDifferent = true;
         }

         BackupGlobalFileNames();
         CoreGlobals.ScenarioDirectory = directory;
         CoreGlobals.TerrainDirectory = directory;
         CoreGlobals.TerrainFile = Path.GetFileName(terrainFile);
         CoreGlobals.ScenarioFile = Path.GetFileName(scenariofilename);
         CoreGlobals.ScenarioLightsetFilename = Path.GetFileName(lightsetfile);         
         
         using (PerfSection p = new PerfSection("PreSaveProject(perforce) " + Path.GetFileName(fileName)))
         {
            SetTopicFiles();
            
            if (savingAsDifferent)
            {
               ClearDoNotSaveFlags();
            }

            string changelistname = CoreGlobals.getScenarioWorkTopicManager().mChangeListPrefix + scenariofilename.Remove(scenariofilename.Length - 4, 4).Remove(0, CoreGlobals.getWorkPaths().mGameScenarioDirectory.Length);
            CoreGlobals.getScenarioWorkTopicManager().SmartInitChangeList(changelistname);

            if (PreSaveEvaluate())
            {
               PopupEditor pe = new PopupEditor();
               ScenarioWorkCoordinator swc = new ScenarioWorkCoordinator();
               if (pe.ShowPopup(this, swc, FormBorderStyle.FixedDialog, true).ShowDialog() != DialogResult.OK)
               {
                  //Revert FileNames refresh perforce settings
                  RestoreGlobalFileNames();
                  SetTopicFiles();
                  mScenarioSourceControl2.Reset();
                  return;
               }

               mScenarioSourceControl2.UpdateUI();

            }
            if (mLastName != scenariofilename)
            {
               mScenarioSourceControl2.Reset();
               mLastName = scenariofilename;
            }
            this.Parent.Text = Path.GetFileNameWithoutExtension(mLastName);
         }

         using (PerfSection p = new PerfSection("SaveProject " + Path.GetFileName(fileName)))
         {


            if (workTopics["Terrain"].DoNotSave == false && workTopics["Terrain"].Changed)
            {
               using (PerfSection p2 = new PerfSection("mTerrainEdit.SaveProject()"))
               {
                  mTerrainEdit.SaveProject(scenariofilename);  //yes, still give it the scenario name. didn't want to refactor too much
               }
            }

            if (workTopics["Lights"].DoNotSave == false && workTopics["Lights"].Changed)
            {
               using (PerfSection p3 = new PerfSection("Savelights"))
               {
 
                  if(File.Exists(lightsetfile) == false)
                  {
                     string defaultlights = CoreGlobals.getWorkPaths().GetDefaultLightset();
                     string destinationLights = Path.Combine(directory, Path.GetFileNameWithoutExtension(scenariofilename) + ".gls");
                     File.Copy(defaultlights, destinationLights);                   
                  }

                  this.mTempLightSettings.saveXML(lightsetfile);
               }
            }

            if (workTopics["Sim"].DoNotSave == false && workTopics["Sim"].Changed)
            {

               using (PerfSection p1 = new PerfSection("BakeTriggerData"))
               {
                  bool bakeData = true;
                  if (CoreGlobals.OutOfMemory == true)
                  {
                     CoreGlobals.ShowMessage("If you are working on trigger scripts you will need to re-save after reloading the editor.");
                     bakeData = false;   
                  }
                  if (bakeData)
                  {
                     BakeTriggerData();
                  }
               }
               using (PerfSection p1 = new PerfSection("SaveSim"))
               {
                  SimGlobals.getSimMain().SaveScenario(scenariofilename, terrainFile, lightsetfile);
               }
            }
            if (workTopics["ArtObjects"].DoNotSave == false && workTopics["ArtObjects"].Changed)
            {              
               SimGlobals.getSimMain().SaveScenarioArtObjects(Path.ChangeExtension(scenariofilename,"sc2"));
            }
            if (workTopics["Sounds"].DoNotSave == false && workTopics["Sounds"].Changed)
            {
               SimGlobals.getSimMain().SaveScenarioSoundObjects(Path.ChangeExtension(scenariofilename,"sc3"));
            }

            if (workTopics["Masks"].DoNotSave == false && workTopics["Masks"].Changed)
            {

               using (PerfSection p4 = new PerfSection("mScenarioSettings"))
               {
                  string masksFileName = Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioEditorSettingsFilename);
                  if (CoreGlobals.FatalSCNSaveError == false && CoreGlobals.FatalEdDataSaveError == false)
                  {
                     FileUtilities.BackupScenarioFile(masksFileName);
                  }
                  try
                  {
                     if (mScenarioSettings == null)
                     {
                        mScenarioSettings = new ZipStreamArchive();

                        //Bind mask control
                        if (CoreGlobals.getEditorMain().mIMaskPickerUI == null)
                        {
                           CoreGlobals.getEditorMain().mIGUI.ShowDialog("MaskLayers");
                        }
                        mScenarioSettings.RegisterStream((IDataStream)CoreGlobals.getEditorMain().mIMaskPickerUI);
                     }
                     mScenarioSettings.SaveData(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioEditorSettingsFilename), true, false);
                  }
                  catch (System.Exception ex)
                  {
                     CoreGlobals.FatalEdDataSaveError = true;
                     throw ex;
                  }
               }
            }
            //using (PerfSection p5 = new PerfSection("UpdatePerforcePage"))
            //{
            //   UpdatePerforcePage();
            //}
         }
      }

      void InitDepartmentCombo()
      {
         mbPauseDepartmentCombo = true;

         int mode = mSimMain.GetCurrentDepartmentMode();
         //if(mode == ()eDepartment.Design
         if (mode == (int)eDepartment.Design)
         {
            departmentModeComboBox.SelectedIndex = 0;
         }
         if (mode == (int)eDepartment.Art)
         {
            departmentModeComboBox.SelectedIndex = 1;
         }
         if (mode == (int)eDepartment.Sound)
         {
            departmentModeComboBox.SelectedIndex = 2;
         }

         mbPauseDepartmentCombo = false;
      }
      bool mbPauseDepartmentCombo = false;
      void departmentModeComboBox_SelectedIndexChanged(object sender, System.EventArgs e)
      {
         int oldValue = mSimMain.GetCurrentDepartmentMode();

         int value = (int)eDepartment.Design;
         if(departmentModeComboBox.SelectedItem.ToString().Contains("Design"))
         {
            value = (int)eDepartment.Design;
         }
         if (departmentModeComboBox.SelectedItem.ToString().Contains("Art"))
         {
            value = (int)eDepartment.Art;
         } 
         if (departmentModeComboBox.SelectedItem.ToString().Contains("Sound"))
         {
            value = (int)eDepartment.Sound;
         }

         mSimMain.SetCurrentDepartmentMode(value);

         if (mbPauseDepartmentCombo == true)
            return;
         //?
         if (mSimMain.CheckDepartmentPermission() == false)
         {
            mSimMain.SetCurrentDepartmentMode(oldValue);
            InitDepartmentCombo();
         }
      }

      void disableSmartHideButton_Click(object sender, System.EventArgs e)
      {
         disableSmartHideButton.Checked = !disableSmartHideButton.Checked;

         mSimMain.mbDisableSmartHide = disableSmartHideButton.Checked;
     
      }

      ZipStreamArchive mScenarioSettings = null;

      public void LoadProjectSimData(string fileName)
      {
        

         using (PerfSection p2 = new PerfSection("tempLightSettings1.loadXML"))
         {
            ProgressHelper.StatusMessage("GLS Load");
            this.mTempLightSettings.loadXML(Path.Combine(Path.GetDirectoryName(fileName), Path.GetFileNameWithoutExtension(fileName) + ".gls"));
         }

         using (PerfSection p3 = new PerfSection("LoadScenarioPlayerData"))
         {
            ProgressHelper.StatusMessage("LoadScenarioPlayerData");
            LoadScenarioPlayerData();
         }

         using (PerfSection p4 = new PerfSection("LoadObjectivesData"))
         {
            ProgressHelper.StatusMessage("LoadObjectivesData");
            LoadObjectivesData();
         }

         using (PerfSection p5 = new PerfSection("LoadTriggerData"))
         {
            ProgressHelper.StatusMessage("LoadTriggerData");
            LoadTriggerData();
         }

         using (PerfSection p6 = new PerfSection("tempLightSettings1.reload()"))
         {
            ProgressHelper.StatusMessage("GLS Reload");
            this.mTempLightSettings.reload();
         }

         using (PerfSection p7 = new PerfSection("MaskLayer init"))
         {
            if (mScenarioSettings == null)
            {
               ProgressHelper.StatusMessage("MaskLayers Load");

               mScenarioSettings = new ZipStreamArchive();

               //Bind mask control
               if (CoreGlobals.getEditorMain().mIMaskPickerUI == null)
               {
                  CoreGlobals.getEditorMain().mIGUI.ShowDialog("MaskLayers");
               }
               mScenarioSettings.RegisterStream((IDataStream)CoreGlobals.getEditorMain().mIMaskPickerUI);
            }
         }

         using (PerfSection p8 = new PerfSection("mScenarioSettings.LoadData"))
         {
            ProgressHelper.StatusMessage("mScenarioSettings.LoadData");
            mScenarioSettings.LoadData(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioEditorSettingsFilename));
         }

      }

      public void LoadProject(string fileName)
      {
         using (PerfSection p = new PerfSection("LoadProject"))
         {


            ProgressHelper.StatusMessage("Loading " + fileName);
            mDoingLoad = true;


            using (PerfSection p1 = new PerfSection("mTerrainEdit.LoadProject"))
            {
               ProgressHelper.StatusMessage("start mTerrainEdit.LoadProject");
               mTerrainEdit.LoadProject(fileName);
            }

            using (PerfSection p2 = new PerfSection("LoadProjectSimData"))
            {
               LoadProjectSimData(fileName);
            }

            mTerrainEdit = TerrainGlobals.getTerrainFrontEnd();
            mSimMain = SimGlobals.getSimMain();

            

            //using (PerfSection p9 = new PerfSection("Perforce"))
            //{
            //   //This Part is dead
            //   ProgressHelper.StatusMessage("Perforce Init");
            //   UpdatePerforceIcon(fileName);
            //   UpdatePerforcePage();

            //}

            mSimMain.mSimFileData.CreateScenarioExtrasIfMissing(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioArtFile));
            mSimMain.mSimFileData.CreateScenarioExtrasIfMissing(Path.Combine(CoreGlobals.ScenarioDirectory,CoreGlobals.ScenarioSoundFile));
            
            NewScenarioTopics();
            SetTopicFiles();

            using (PerfSection p10 = new PerfSection("Perforce - CleanEmptyChangeLists"))
            {
               //this part is good but may be slow
               if (CoreGlobals.UsingPerforce == true)
               {
                  CoreGlobals.getPerforce().CleanEmptyChangeLists(CoreGlobals.getScenarioWorkTopicManager().mChangeListPrefix);

                  string scenariofilename = fileName;
                  string changelistname = CoreGlobals.getScenarioWorkTopicManager().mChangeListPrefix + scenariofilename.Remove(scenariofilename.Length - 4, 4).Remove(0, CoreGlobals.getWorkPaths().mGameScenarioDirectory.Length);
                  CoreGlobals.getScenarioWorkTopicManager().SmartInitChangeList(changelistname);

               }
            }
            using (PerfSection p10b = new PerfSection("Perforce - Update initial state"))
            {
               if (CoreGlobals.UsingPerforce == true)
               {
                  CoreGlobals.getScenarioWorkTopicManager().InitFileLoadRevisions();
               }
            }

            mbHasClickedQuickView = false;
            mDoingLoad = false;

            ResetCamera();

            ProgressHelper.Finished(true);

            timer1.Enabled = CoreGlobals.getSettingsFile().AutoSaveEnabled;
            timer1.Interval = CoreGlobals.getSettingsFile().AutoSaveTimeInMinutes * 60000;
            timer2.Interval = 600000;



         }
      }

      public void LoadTriggerData()
      {
         //Cleanup
         List<TabPage> hitlist = new List<TabPage>();
         mlScenarioScriptsManager.ClearData();
         foreach (TabPage p in TabControl.Controls)
         {
            TriggerEditor ed = p.Tag as TriggerEditor;

            if (ed != null)
            {
               hitlist.Add(p);
            }
         }
         foreach (TabPage p in hitlist)
         {
            TabControl.Controls.Remove(p);
         }


         if (SimGlobals.getSimMain().TriggerData.Count == 0)
         {
            SimGlobals.getSimMain().TriggerData.Add(new TriggerRoot());

         }
         //this.mFirstTriggerEditor.TriggerData = SimGlobals.getSimMain().TriggerData;

         foreach (TriggerRoot root in SimGlobals.getSimMain().TriggerData)
         {
            if (root.External == false)
            {
               mlScenarioScriptsManager.AddNewTab(root);
            }
         }
        
         mlScenarioScriptsManager.UpdateData();


      }

      //public void UpdatePerforcePage()
      //{
      //   this.mScenarioSourceControl.Enabled = false;
      //   return;

      //   if (CoreGlobals.UsingPerforce)
      //   {
      //      List<string> files = new List<string>();
      //      files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioFile));
      //      files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioFile + ".xmb"));
      //      files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.TerrainFile));
      //      files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioLightsetFilename));
      //      files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioLightsetFilename + ".xmb"));

      //      files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioEditorSettingsFilename));

      //      files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "xtd")));
      //      files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "xtt")));
      //      files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "xth")));

      //      files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.ScenarioFile, "xsd")));
      //      files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.ScenarioFile, "lrp")));

      //      files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "dep")));
      //      files.Add(Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "dep.xmb")));

      //      files.Add(Path.Combine(CoreGlobals.ScenarioDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "tag")));
            


      //      //export?
      //      this.mScenarioSourceControl.Enabled = true;
      //      this.mScenarioSourceControl.SetFileList(files);

      //      UpdatePerforceIcon(files[0]);
      //   }
      //}

      public void ResetP4Icon()
      {
         PerforceTabPage.ImageIndex = 3;
      }

      public void UpdatePerforceIcon(string fileName)
      {
         if (CoreGlobals.UsingPerforce == true)
         {
            SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(fileName);

            if (status.InPerforce == false)
               PerforceTabPage.ImageIndex = 3;
            else if (status.CheckedOutThisUser == true)
               PerforceTabPage.ImageIndex = 5;
            else if (status.CheckedOutOtherUser == true)
               PerforceTabPage.ImageIndex = 6;
            else if (status.CheckedOut == false)
               PerforceTabPage.ImageIndex = 4;
         }
      }

      public bool CheckPerforceLoad(string fileName)
      {
         try
         {
            if (CoreGlobals.UsingPerforce == true)
            {
               SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(fileName);
               if (status.CheckedOutOtherUser == true)
               {
                  if (MessageBox.Show("This scenario is checked out by " + status.ActionOwner + " open anyway?", "", MessageBoxButtons.OKCancel) == DialogResult.OK)
                  {
                     return true;
                  }
                  else
                  {
                     return false;
                  }
               }

            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.UsingPerforce = false;
         }
         return true;
      }


      public bool UnloadScenario()
      {
         return true;
      }
      

      #endregion 




      private void LoadScenarioPlayerData()
      {
         WorkTopic topic = null;
         if (CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics().TryGetValue("Sim", out topic))
         {
            topic.Paused = true;
         }
         mPlayerSettings.SimMain = mSimMain;
         if (topic != null)
         {
            topic.Paused = false;
         }

      }

      private void LoadObjectivesData()
      {
         mObjectivesControl.InitializeObjectivesListBox();
      }

      //EVENTS FROM THE PANEL
      private void ScenarioEditorPage_Resize(object sender, EventArgs e)
      {
         mbSuppresDeviceAutoSave = true;
         //if (PreventDeviceResize == true)
         //   return;
         BRenderDevice.changePerspectiveParams(40, GetSubTarget().Width, GetSubTarget().Height);


         Point p1 = this.PointToScreen(new Point(0, 0));
         Point p2 = CoreGlobals.getEditorMain().mIGUI.getWindowLocation();
         BRenderDevice.setWindowOffset(p1.X - p2.X, p1.Y - p2.Y);

         //if (!PreventDeviceResize && (mLastWidth < GetSubTarget().Width || mLastHeight < GetSubTarget().Height))
         if (!PreventDeviceResize && (mLastWidth != GetSubTarget().Width || mLastHeight != GetSubTarget().Height))
         {
            if (mLastWidth != -1)
            {

               //this call passes in the panel width and height
               CoreGlobals.getEditorMain().mIGUI.deviceResize(GetSubTarget().Width, GetSubTarget().Height, false);
               //BRenderDevice.changePerspectiveParams(40, this.Width, this.Height);
               BRenderDevice.changePerspectiveParams(40, GetSubTarget().Width, GetSubTarget().Height);
               BRenderDevice.setWindowOffset(GetSubTarget().Left, GetSubTarget().Top);
            }

            mLastHeight = GetSubTarget().Height;
            mLastWidth = GetSubTarget().Width;

         }


         //if (CoreGlobals.getEditorMain().mIGUI.getActiveClientPage() == this)
         //{
         //   MainWindow.mMainWindow.deviceResize(this.Width, this.Height, false);
         //   BRenderDevice.changePerspectiveParams(40, this.Width, this.Height);
         //}

         mbSuppresDeviceAutoSave = false;

      }

      #region Main Menu
    

      private void toggleSimOBJRenderingToolStripMenuItem_Click(object sender, EventArgs e)
      {
         try
         {
            mSimMain.toggleRenderSimObjects();
            toggleSimOBJRenderingToolStripMenuItem.Checked = !toggleSimOBJRenderingToolStripMenuItem.Checked;
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      private void chunkBoundsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         try
         {
            chunkBoundsToolStripMenuItem.Checked = !chunkBoundsToolStripMenuItem.Checked;
            mTerrainEdit.ToggleEChunkBounds();

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      private void dynamicLODToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getVisual().setDynamicLODEnabled(true);//!TerrainGlobals.getVisual().isDynamicLODEnabled());

         dynamicLODToolStripMenuItem.Checked = true;
         minLODToolStripMenuItem.Checked = false;
         maxLODToolStripMenuItem.Checked = false;
      }

      private void maxLODToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getVisual().setDynamicLODEnabled(false);
         TerrainGlobals.getVisual().setStaticLODLevel(BTerrainVisual.eLODLevel.cLOD0);

         dynamicLODToolStripMenuItem.Checked = false;
         minLODToolStripMenuItem.Checked = false;
         maxLODToolStripMenuItem.Checked = true;
      }

      private void minLODToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getVisual().setDynamicLODEnabled(false);
         TerrainGlobals.getVisual().setStaticLODLevel(BTerrainVisual.eLODLevel.cLOD3);

         dynamicLODToolStripMenuItem.Checked = false;
         minLODToolStripMenuItem.Checked = true;
         maxLODToolStripMenuItem.Checked = false;
      }

      private void rTSViewToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mTerrainEdit.snapCamera(BCameraManager.eCamSnaps.cCamSnap_RTS);
      }

      private void rTSModeedgePushToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mTerrainEdit.ToggleEdgeMove();
         rTSModeedgePushToolStripMenuItem.Checked = !rTSModeedgePushToolStripMenuItem.Checked;

      }

      private void yToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mTerrainEdit.snapCamera(BCameraManager.eCamSnaps.cCamSnap_posY);
      }

      private void xToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mTerrainEdit.snapCamera(BCameraManager.eCamSnaps.cCamSnap_posX);
      }

      private void xToolStripMenuItem1_Click(object sender, EventArgs e)
      {
         mTerrainEdit.snapCamera(BCameraManager.eCamSnaps.cCamSnap_negX);
      }

      private void zToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mTerrainEdit.snapCamera(BCameraManager.eCamSnaps.cCamSnap_posZ);
      }

      private void zToolStripMenuItem1_Click(object sender, EventArgs e)
      {
         mTerrainEdit.snapCamera(BCameraManager.eCamSnaps.cCamSnap_negZ);
      }

      private void exportOBJToolStripMenuItem_Click(object sender, EventArgs e)
      {
         ExportToOBJ eto = new ExportToOBJ();
         eto.ShowDialog();
      }

      private void exportSelectedOBJToolStripMenuItem_Click(object sender, EventArgs e)
      {
        
      }

      private void maskLayersToolStripMenuItem_Click(object sender, EventArgs e)
      {
         MainWindow.mMainWindow.ShowDialog("MaskLayers");

      }
      private void texturingPanelToolStripMenuItem_Click(object sender, EventArgs e)
      {
         MainWindow.mMainWindow.ShowDialog("TexturingPanel");
      }

      private void invertToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mTerrainEdit.mCameraManager.mbInvertZoom = !mTerrainEdit.mCameraManager.mbInvertZoom;
         invertToolStripMenuItem.Checked = mTerrainEdit.mCameraManager.mbInvertZoom;
      }

      private void ShowWorldObjectListMenuItem_Click(object sender, EventArgs e)
      {
         MainWindow.mMainWindow.ShowDialog("WorldObjectList");

      }
      #endregion


      #region Basic ToolStrip
      private void newToolStripButton_Click(object sender, EventArgs e)
      {
         //try
         //{
         //   NewScenario(256);
         //}
         //catch (System.Exception ex)
         //{
         //   CoreGlobals.getErrorManager().OnException(ex);
         //}
      }
      private void pasteToolStripButton_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getTerrainFrontEnd().PasteMode();
      }
      private void copyToolStripButton_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getTerrainFrontEnd().Copy();
      }
      private void cutToolStripButton_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getTerrainFrontEnd().Cut(false);
      }
      private void uppercutToolStripButton_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getTerrainFrontEnd().Cut(true);
      }
      private void UndoToolStripButton_Click(object sender, EventArgs e)
      {
         try
         {
            MainWindow.mMainWindow.EditorPause();
            UIManager.Pause();

            mTerrainEdit.Undo();

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            MainWindow.mMainWindow.EditorUnPause();
            UIManager.UnPause();
         }
      }
      private void undoToolStripMenuItem_Click(object sender, EventArgs e)
      {
         try
         {
            MainWindow.mMainWindow.EditorPause();
            UIManager.Pause();
            mTerrainEdit.Undo();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            MainWindow.mMainWindow.EditorUnPause();
            UIManager.UnPause();
         }

      }
      private void RedoToolStripButton_Click(object sender, EventArgs e)
      {
         try
         {
            MainWindow.mMainWindow.EditorPause();
            UIManager.Pause();
            mTerrainEdit.Redo();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            MainWindow.mMainWindow.EditorUnPause();
            UIManager.UnPause();
         }

      }


      private void helpToolStripButton_Click(object sender, EventArgs e)
      {
         //string help = "----CONTROLS----\n" +
         //"F1 \t- HELP\n" +
         //"WASD \t- Pan World\n" +
         //"SHIFT + WASD \t- Rotate around visible chunks \n" +
         //"SHIFT + MouseWheel \t- Zoom \n" +
         //"SPACE \t- reset to game camera\n" +
         //"Q \t- Toggle screen edge movement\n" +
         //"\n" +
         //"\nVertexMode:------------------------\n" +
         //"F \t- Height Brush\n" +
         //"R \t- Push Brush \n" +
         //"E \t- Smooth Brush (Height only)\n" +
         //"V \t- Cycle brush Shape (vertex mode)\n" +
         //"Right SHIFT \t- Lock X AXIS\n" +
         //"Right CONTROL \t- Lock Z AXIS\n" +
         //"Right ALT \t\t- Lock Y AXIS\n" +
         //"\nControlMode:------------------------\n" +
         //"C \t- Enter Control Mode\n" +
         //"LCLICK \t- Drag to move selected verts\n" +
         //"RCLICK \t- Drag to select verts. (SHIFT-continue select)\n" +
         //"\nTexureMode:------------------------\n" +
         //"Z \t- Splat Brush\n " +
         //"X \t- Decal Brush \n" +
         //"L/R \t- Cycle active texture in working set\n" +
         //"U/D \t- Cycle active texture masks\n" +
         //"\n" +
         //"MouseWheel \t- Change brush size (all modes) \n" +
         //"LCTRL+MouseWheel \t- Change the step amount in Vertex Modes \n" +
         //"\n" +
         //"1 - Textured Lit Mode\n" +
         //"2 - Wireframe Textured Lit Mode\n" +
         //"3 - Lit Only\n" +
         //"4 - Wireframe Lit Only\n" +
         //"5 - Wireframe overlay\n";
         //MessageBox.Show(help);


         CoreGlobals.getEditorMain().mIGUI.ShowScenarioOptions();
      }


      //private void loadMskImg_Click(object sender, EventArgs e)
      //{
      //   try
      //   {
      //      EditorPause();
      //      UIManager.Pause();

      //      OpenFileDialog d = new OpenFileDialog();
      //      d.Filter = "32bit BMP files (*.BMP)|*.BMP";
      //      d.FilterIndex = 0;
      //      d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mBrushMasks;

      //      if (d.ShowDialog() == DialogResult.OK)
      //      {
      //         mTerrainEdit.LoadMaskTexture(d.FileName);
      //         CoreGlobals.getSaveLoadPaths().mBrushMasks = Path.GetDirectoryName(d.FileName);
      //      }            
      //   }
      //   catch(System.Exception ex)
      //   {
      //      CoreGlobals.getErrorManager().OnException(ex);
      //   }
      //   finally
      //   {
      //      EditorUnPause();
      //      UIManager.UnPause();
      //   }
      //}


      private void RenderModeToolStripComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         try
         {
            if (mTerrainEdit == null) return;
            if (RenderModeToolStripComboBox.Text.Contains("Render Full Wireframe"))
            {
               mTerrainEdit.setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderFullWireframe);
               TerrainGlobals.getTexturing().reloadCachedVisuals();
            }
            else if (RenderModeToolStripComboBox.Text.Contains("Render Flat Wireframe"))
            {
               mTerrainEdit.setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderFlatWireframe);
            }
            else if (RenderModeToolStripComboBox.Text.Contains("Render Flat"))
            {
               mTerrainEdit.setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderFlat);
            }
            else if (RenderModeToolStripComboBox.Text.Contains("Render Full Overlay"))
            {
               mTerrainEdit.setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderFullOverlay);
            }
            else if (RenderModeToolStripComboBox.Text.Contains("Render Full"))
            {
               mTerrainEdit.setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderFull);
            }
            else if (RenderModeToolStripComboBox.Text.Contains("Render Selected Texture"))
            {
               mTerrainEdit.setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderTextureSelectRender);
               TerrainGlobals.getTexturing().reloadCachedVisuals();
            }
            else if (RenderModeToolStripComboBox.Text.Contains("Ambient Occlusion"))
            {
               mTerrainEdit.setRenderMode(BTerrainEditor.eEditorRenderMode.cRenderAmbientOcclusion);
            }
            else if (RenderModeToolStripComboBox.Text.Contains("Texture Perf Eval"))
            {
               mTerrainEdit.setRenderMode(BTerrainEditor.eEditorRenderMode.cTerrainPerfEval);
            }
            else if (RenderModeToolStripComboBox.Text.Contains("Vertex Perf Eval"))
            {
               TerrainGlobals.getEditor().viewLODEval();
            }

            RenderModeToolStripComboBox.Invalidate();

            MainWindow.mMainWindow.SetClientFocus();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }


      }
      private void quickViewButton_Click(object sender, EventArgs e)
      {
         BakeTriggerData();
         doQuickView();
      }

      private void clearAOButton_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().clearAmbientOcclusion();
      }
      private void calcAOButton_Click(object sender, EventArgs e)
      {
         Terrain.Controls.AOGenDialog daog = new Terrain.Controls.AOGenDialog();
         daog.Show();
      }



      #endregion 

      
      #region Active Sync to 360 stuff
     
      public void doQuickSave(string coreName)
      {
         doQuickSave(CoreGlobals.getWorkPaths().mGameScenarioDirectory,coreName);
      }
      public void doQuickSave(string scenerioPath, string coreName)
      {

         string changelistname = CoreGlobals.getScenarioWorkTopicManager().mChangeListPrefix + coreName;
         CoreGlobals.getScenarioWorkTopicManager().SmartInitChangeList(changelistname);

         if ((coreName != "quickview") && PreSaveEvaluate())
         {
            PopupEditor pe = new PopupEditor();
            ScenarioWorkCoordinator swc = new ScenarioWorkCoordinator();
            if (pe.ShowPopup(this, swc, FormBorderStyle.FixedDialog, true).ShowDialog() != DialogResult.OK)
            {
               return;
            }
         }


         if (scenerioPath == "")
            scenerioPath = CoreGlobals.getWorkPaths().mGameScenarioDirectory;
         if (scenerioPath.Contains(coreName))
            scenerioPath = scenerioPath.Remove(scenerioPath.IndexOf(coreName),coreName.Length);
         if (!Directory.Exists(scenerioPath + @"\" + coreName))
            Directory.CreateDirectory(scenerioPath + @"\" + coreName);
         try
         {
            string[] fls = null;
            string quickfls = scenerioPath + @"\" + coreName + @"\" + coreName + ".fls";

            if (File.Exists(quickfls))
            {
               File.SetAttributes(quickfls, FileAttributes.Normal);
               File.Delete(quickfls);
            }

            if (CoreGlobals.TerrainDirectory != null && CoreGlobals.TerrainDirectory != "")
            {
               fls = Directory.GetFiles(CoreGlobals.TerrainDirectory, CoreGlobals.GetBaseProjectName() + ".fls");
            }
            if (fls != null && fls.Length == 1)
            {
               File.Copy(fls[0], quickfls, true);
            }

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }


         bool bSimOnly = simOnlyToolStripMenuItem.Checked;
         bool bNoScript = noScriptToolStripMenuItem.Checked;

         string scenarioName = scenerioPath + @"\" + coreName + @"\" + coreName + ".scn";
         string terrainName = Path.ChangeExtension(scenarioName, "ted");
         string lightsetName = Path.ChangeExtension(scenarioName, "gls");

         if (!bSimOnly)
         {
            mTerrainEdit.SaveProject(scenarioName);
         }

         SimGlobals.getSimMain().SaveScenario(scenarioName, terrainName, lightsetName, bNoScript);
         SimGlobals.getSimMain().SaveExtasMacro(scenarioName);

         this.mTempLightSettings.saveXML(lightsetName);

      }
      public void doQuickView()
      {
         mbHasClickedQuickView = true;

         doQuickSave("quickview");

         bool bSimOnly = simOnlyToolStripMenuItem.Checked;
         bool bNoScript = noScriptToolStripMenuItem.Checked;
         bool bRelaunchGame = relaunchGameToolStripMenuItem.Checked;

         try
         {
            string scenerioPath = CoreGlobals.getWorkPaths().mGameScenarioDirectory;

            Export360.ExportSettings es = new Export360.ExportSettings();
            TerrainGlobals.getTerrain().getExportSettings().copyTo(ref es);
            if (bSimOnly == false)
            {
               //CLM quickview sholdn't bone with the export settings dialog

               if (mExportDialog.mExportSettings == null)
                  mExportDialog.mExportSettings = TerrainGlobals.getTerrain().getExportSettings();
               mExportDialog.mExportSettings.SettingsQuick();
               mExportDialog.mExportSettings.AmbientOcclusion = ambientOcclusionToolStripMenuItem.Checked ? Terrain.AmbientOcclusion.eAOQuality.cAO_Worst : Terrain.AmbientOcclusion.eAOQuality.cAO_Off;
               mExportDialog.mExportSettings.RefineTerrain = refineTerrainToolStripMenuItem.Checked;
               mExportDialog.mExportSettings.NavMesh = false;

               ExportXTD(scenerioPath + @"\quickview\quickview.XTD", scenerioPath + @"\quickview\quickview.XSD", false);
            }
            TerrainGlobals.getTerrain().setExportSettings(es);
            es = null;

            if (XFSInterface.launchApp())
            {
               if (bRelaunchGame)
               {
                  XFSInterface.launchGame();
               }
               XFSInterface.launchScenario(@"\quickview\quickview");
            }

            CoreGlobals.getEditorMain().mIGUI.afterQuickView();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         
      }

      bool mbLaunchGameEachTime = true;

      void stopQuickView()
      {
        
      }
      #endregion

      #region Export Code
      public void ExportXTD(string terrainFileName,  bool doConf)
      {
         ExportXTD(terrainFileName, terrainFileName, doConf);
      }
      public void ExportXTD(string terrainFileName, string scenarioFileName, bool doConf)
      {
         string xtd = ".XTD";
         string nme = Path.GetFileNameWithoutExtension(terrainFileName);
         string pth = Path.GetDirectoryName(terrainFileName);
         string filePath = pth + "\\";

         

         ExporterResults resDLG = new ExporterResults();
         //if we're an ALT scenario, then turn off everything but XSD
         if(Path.GetDirectoryName(terrainFileName) != Path.GetDirectoryName(scenarioFileName))
            ExportTo360.doExport(terrainFileName, scenarioFileName, null, ref mExportDialog.mExportSettings, ref resDLG.results,false, false, false, true,true, false,true, true);
         else
            ExportTo360.doExport(terrainFileName, scenarioFileName, ref mExportDialog.mExportSettings, ref resDLG.results);


       
         if (doConf)
            resDLG.ShowDialog() ;
         return;
      }
      ExportDialog mExportDialog = new ExportDialog();
 
      private void exportTo360ToolStripMenuItem_Click(object sender, EventArgs e)
      {
         DoExport();
      }

      private void DoExport()
      {
         try
         {

            if (mExportDialog.ShowDialog() == DialogResult.OK)
            {




               MainWindow.mMainWindow.EditorPause();
               UIManager.Pause();

               string fileName = CoreGlobals.TerrainFile;
               if (fileName == "")
               {
                  if (MessageBox.Show("The scenario must be saved first", "", MessageBoxButtons.OKCancel) == DialogResult.OK)
                  {
                     save_file_as();// file_saveas_Click(sender, e);
                  }
                  else
                  {
                     return;
                  }
               }


               if (PreExportEvaluate())
               {
                  PopupEditor pe = new PopupEditor();
                  ExportWorkCoordinator swc = new ExportWorkCoordinator();
                  
                  if (pe.ShowPopup(this, swc, FormBorderStyle.FixedDialog, true).ShowDialog() != DialogResult.OK)
                  {
                     mScenarioSourceControl2.UpdateUI();
                     return;
                  }
                  mScenarioSourceControl2.UpdateUI();

               }
               



               fileName = Path.Combine(CoreGlobals.TerrainDirectory, Path.ChangeExtension(CoreGlobals.TerrainFile, "XTD"));
               string scnName = Path.Combine(CoreGlobals.ScenarioDirectory, Path.ChangeExtension(CoreGlobals.ScenarioFile, "XSD"));
               ExportXTD(fileName, scnName, true);

            }
            else
            {
               return;

            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
         finally
         {
            MainWindow.mMainWindow.EditorUnPause();
            UIManager.UnPause();
         }

      }
   

      private void quickviewOn360ToolStripMenuItem_Click_1(object sender, EventArgs e)
      {
         doQuickView();
      }

      #endregion

      private void maskLayersToolStripMenuItem_Click_1(object sender, EventArgs e)
      {
         MainWindow.mMainWindow.ShowDialog("MaskLayers");
      }
      private void noiseSettingsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         MainWindow.mMainWindow.ShowDialog("NoiseSettings");
      }
      private void brushSettingsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         MainWindow.mMainWindow.ShowDialog("BrushSettings");
      }
      private void tabletSetttingsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         MainWindow.mMainWindow.ShowDialog("TabletSettings");
      }

      private void toolStripContainer1_TopToolStripPanel_Click(object sender, EventArgs e)
      {

      }

      private void startActiveSyncToolStripMenuItem_Click(object sender, EventArgs e)
      {
        
        
         //startActiveSyncToolStripMenuItem.Checked = !startActiveSyncToolStripMenuItem.Checked;
     //    setActiveSyncEnabled(startActiveSyncToolStripMenuItem.CheckState == CheckState.Checked);
      }

      private void tabControl1_KeyPress(object sender, KeyPressEventArgs e)
      {
         //e.Handled = true;
      }

      private void tabControl1_KeyDown(object sender, KeyEventArgs e)
      {
         e.Handled = true;
      }

      private void panel1_Click(object sender, EventArgs e)
      {
         MainWindow.mMainWindow.SetClientFocus();
      }

      private void tabControl1_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (DoRealTimeStuff())
         {
            Activate();
         }
         UIManager.RedirectSubControlUI(GetSubTarget());

         if (TabControl.SelectedTab == scenarioDataTabPage)
         {
            ((ScenarioSettings)scenarioDataTabPage.Controls[0]).isActive();

         }
      }

      private void toolStripContainer1_ContentPanel_Load(object sender, EventArgs e)
      {

      }

      private void terrainVertsCursorToolStripMenuItem_Click(object sender, EventArgs e)
      {
         ////////CHECK ON CLICK PLEASE!
         //terrainVertsCursorToolStripMenuItem.Checked = !terrainVertsCursorToolStripMenuItem.Checked;
         TerrainGlobals.getEditor().enableShowVerts(terrainVertsCursorToolStripMenuItem.Checked);
      }

      private void designerControlsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         ////////CHECK ON CLICK PLEASE!
         //designerControlsToolStripMenuItem.Checked = !designerControlsToolStripMenuItem.Checked;
         SimGlobals.getSimMain().useDesignerControls(designerControlsToolStripMenuItem.Checked);

         CoreGlobals.getSettingsFile().DesignerControls = designerControlsToolStripMenuItem.Checked;
         CoreGlobals.getSettingsFile().Save();
      }

      private void reloadVisibleObjectsToolStripMenuItem_Click(object sender, EventArgs e)
      {

         ReloadVisibleObjects();
      }

      public void ReloadVisibleObjects()
      {
         try
         {

            SimGlobals.getSimMain().ReloadSimData();

            SimGlobals.getSimMain().ReloadVisible();

            MainWindow.mMainWindow.mUnitPicker.ReLoadData();

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }
   


      private void terrainVertsCursorToolStripMenuItem_Click_1(object sender, EventArgs e)
      {
         terrainVertsCursorToolStripMenuItem.Checked = !terrainVertsCursorToolStripMenuItem.Checked;
         TerrainGlobals.getEditor().toggleShowVerts();
      }

      // IUnitPicker interface functions
      //
      public void PlayerIdSelectedChanged(int index)
      {
         SimGlobals.getSimMain().SelectedPlayerID = index;
         SimGlobals.getSimMain().changeSelectedItemsOwningPlayer();
      }

      public void UnitSelectedChanged(object obj)
      {
         SimUnitXML unit = obj as SimUnitXML;
         if (unit != null)
         {
            SimGlobals.getSimMain().SelectedProtoUnit = unit;
            SimGlobals.getSimMain().PlaceItemMode();
         }
      }

      #region ISquadPicker Members

      public void SquadSelectedChanged(ProtoSquadXml squad)
      {
         if (squad != null)
         {
            SimGlobals.getSimMain().SelectedProtoSquad = squad;
            SimGlobals.getSimMain().PlaceSquadMode();

         }

      }

      #endregion

      private void clipArtToolStripMenuItem_Click(object sender, EventArgs e)
      {
         CoreGlobals.getEditorMain().mIGUI.ShowDialog("ClipArtPicker");
      }
      private void newClipArtToolStripMenuItem_Click(object sender, EventArgs e)
      {
         NewClipArt();
      }

      private void TerrainToolStrip_ItemClicked(object sender, ToolStripItemClickedEventArgs e)
      {

      }

      private void ambientOcclusionToolStripMenuItem_Click(object sender, EventArgs e)
      {
         ambientOcclusionToolStripMenuItem.Checked = !ambientOcclusionToolStripMenuItem.Checked;
      }

      private void refineTerrainToolStripMenuItem_Click(object sender, EventArgs e)
      {
         refineTerrainToolStripMenuItem.Checked = !refineTerrainToolStripMenuItem.Checked;
      }
      private void simOnlyToolStripMenuItem_Click(object sender, EventArgs e)
      {
         simOnlyToolStripMenuItem.Checked = !simOnlyToolStripMenuItem.Checked;
      }
      private void relaunchGameToolStripMenuItem_Click(object sender, EventArgs e)
      {
         relaunchGameToolStripMenuItem.Checked = !relaunchGameToolStripMenuItem.Checked;
      }      
      private void xBoxAutoUpdateObjectsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         xBoxAutoUpdateObjectsToolStripMenuItem.Checked = !xBoxAutoUpdateObjectsToolStripMenuItem.Checked;

      }
      private void quickViewButton_MouseUp(object sender, MouseEventArgs e)
      {
         if(e.Button== MouseButtons.Right)
         {
            Point pos = e.Location;
            UIManager.GetCursorPos(ref pos);
            Point winPoint = CoreGlobals.getEditorMain().mIGUI.getWindowLocation();
            quickViewOptions.Show(pos.X + winPoint.X,pos.Y + winPoint.Y);
         }
      }

      private void renderLightsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         renderLightsToolStripMenuItem.Checked = !renderLightsToolStripMenuItem.Checked;
         SimGlobals.getSimMain().toggleRenderLights();
      }

      private void scenarioKeysToolStripMenuItem_Click(object sender, EventArgs e)
      {
         CoreGlobals.getEditorMain().mIGUI.ShowScenarioOptions();
      }

      private void resetCameraToolStripMenuItem_Click(object sender, EventArgs e)
      {
         ResetCamera();
      }
      private void ResetCamera()
      {
        //public Vector3 Eye = new Vector3(0, 25, 0);
        //public Vector3 LookAt = new Vector3(12, 0, 12);
         //mTerrainEdit.setCameraPos(new Vector3(0, 25, 0));
         //mTerrainEdit.setCameraTarget(new Vector3(12, 0, 12));

         mTerrainEdit.snapCamera(BCameraManager.eCamSnaps.cCamSnap_RTS);

      
      }

      private void fractalTerrainToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Filter_Dialogs.randomTerrain dia = new Filter_Dialogs.randomTerrain();
         dia.Show();
      }

      private void terraceToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Filter_Dialogs.terraceTerrain dia = new Filter_Dialogs.terraceTerrain();
         dia.Show();
      }

      private void mesaToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Filter_Dialogs.mesaTerrain dia = new Filter_Dialogs.mesaTerrain();
         dia.Show();
      }

      private void erosionToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Filter_Dialogs.erosion dia = new Filter_Dialogs.erosion();
         dia.Show();
      }

      private void maskFromSlopeToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Filter_Dialogs.maskfromSlope dia = new Filter_Dialogs.maskfromSlope();
         dia.Show();
      }

      private void deformationCurve1ToolStripButton_Click(object sender, EventArgs e)
      {
         deformationCurve1ToolStripButton.Checked = true;
         deformationCurve2ToolStripButton.Checked = false; 
         deformationCurve3ToolStripButton.Checked = false; 

         TerrainGlobals.getEditor().setVertBrushCurveType(0);
      }

      private void deformationCurve2ToolStripButton_Click(object sender, EventArgs e)
      {
         deformationCurve1ToolStripButton.Checked = false;
         deformationCurve2ToolStripButton.Checked = true;
         deformationCurve3ToolStripButton.Checked = false;

         TerrainGlobals.getEditor().setVertBrushCurveType(1);
      }

      private void deformationCurve3ToolStripButton_Click(object sender, EventArgs e)
      {
         deformationCurve1ToolStripButton.Checked = false;
         deformationCurve2ToolStripButton.Checked = false;
         deformationCurve3ToolStripButton.Checked = true;

         TerrainGlobals.getEditor().setVertBrushCurveType(2);
      }

      private void renderSimHelpersToolStripMenuItem_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().toggleRenderSimHelpers();
      }

      private void lockSimHelpersToolStripMenuItem_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().toggleLockSimHelpers();
      }

      private void showSkirtToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().toggleRenderSkirt();
      }

      private void showFogToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().setRenderFog(showFogToolStripMenuItem.Checked);
         CoreGlobals.getSettingsFile().ShowFog = showFogToolStripMenuItem.Checked;
         CoreGlobals.getSettingsFile().Save();
      }

      private void lockLightsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().toggleLockLights();

      }

      private void showLightingToolStripMenuItem_Click(object sender, EventArgs e)
      {
         showLightingToolStripMenuItem.Checked = !showLightingToolStripMenuItem.Checked;
         TerrainGlobals.getEditor().toggleShowLighting();
      }

      private void simTileDensityToolStripMenuItem_Click(object sender, EventArgs e)
      {
         SimTileQuality stq = new SimTileQuality();
         stq.ShowDialog();
      }

      List<string> mSelectedGroups = new List<string>();
      private void ExportObjectsMenuItem_Click(object sender, EventArgs e)
      {

         FlowLayoutPanel p = new FlowLayoutPanel();

         PopupEditor pe = new PopupEditor();
         //chooser
         OptionChooser op = new OptionChooser();
         List<string> options = new List<string>();
         mSelectedGroups = new List<string>();
         foreach(ObjectGroup o in SimGlobals.getSimMain().ObjectGroups)
         {
            options.Add(o.Name);
         }
         op.SetOptions(options);
         op.BoundSelectionList = mSelectedGroups;

         Button b = new Button();
         b.Text = "Export";
         b.Click+=new EventHandler(b_Click);

         p.Controls.Add(op);
         p.Controls.Add(b);
         p.Height = b.Bottom + 20;
         Form exportObjectsForm = pe.ShowPopup(this, p, FormBorderStyle.SizableToolWindow);

      }

      void  b_Click(object sender, EventArgs e)
      {
         try
         {

            if (mSelectedGroups.Count == 0)
               return;

            SaveFileDialog d = new SaveFileDialog();
            d.Filter = "objects(*.objects)|*.objects";
            d.InitialDirectory = CoreGlobals.ScenarioDirectory;
            if (d.ShowDialog() == DialogResult.OK)
            {
               XmlSerializer s = new XmlSerializer(typeof(ScenarioScrapsXML), new Type[] { });
               Stream st = File.Open(d.FileName, FileMode.Create);
               //s.Serialize(st, scraps);

               ScenarioScrapsXML scraps = SimGlobals.getSimMain().ExportGroups(mSelectedGroups);
               s.Serialize(st, scraps);
               st.Close();

               ((Control)sender).FindForm().Close();

               //Not used by game, no xmb needed

            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
  
      
      }

      ScenarioScrapsXML mImportScraps = null;
      private void ImportObjectsMenuItem_Click(object sender, EventArgs e)
      {
         try
         {
            OpenFileDialog d = new OpenFileDialog();
            d.Filter = "objects(*.objects)|*.objects";
            d.InitialDirectory = CoreGlobals.ScenarioDirectory;
            if (d.ShowDialog() == DialogResult.OK)
            {
               XmlSerializer s = new XmlSerializer(typeof(ScenarioScrapsXML), new Type[] { });
               Stream st = File.OpenRead(d.FileName);
               mImportScraps = (ScenarioScrapsXML)s.Deserialize(st);
               st.Close();


               FlowLayoutPanel p = new FlowLayoutPanel();
               PopupEditor pe = new PopupEditor();
               //chooser
               OptionChooser op = new OptionChooser();
               List<string> options = new List<string>();
               mSelectedGroups = new List<string>();
               foreach (ObjectGroup o in mImportScraps.mObjectGroups)
               {
                  options.Add(o.Name);
               }
               op.SetOptions(options);
               op.BoundSelectionList = mSelectedGroups;

               Button b = new Button();
               b.Text = "Import";
               b.Click += new EventHandler(b2_Click);
               

               p.Controls.Add(op);
               p.Controls.Add(b);
               p.Height = b.Bottom + 20;
               Form exportObjectsForm = pe.ShowPopup(this, p, FormBorderStyle.SizableToolWindow);
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         } 

      }
      void b2_Click(object sender, EventArgs e)
      {
         if (mSelectedGroups.Count == 0)
            return;
         SimGlobals.getSimMain().ImportGroups(mImportScraps, mSelectedGroups);

         ((Control)sender).FindForm().Close();
      }

      private void offToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getRender().mbShowGrid = false;
         worldSpaceUnitsToolStripMenuItem1.Checked = false;
         worldSpaceUnitsToolStripMenuItem.Checked = false;
         worldSpaceUnitToolStripMenuItem.Checked = false;
         offToolStripMenuItem.Checked = true;
      }

      private void worldSpaceUnitToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getRender().mbShowGrid = true;
         TerrainGlobals.getRender().mGridSpacing = 1.0f;
         worldSpaceUnitsToolStripMenuItem1.Checked = false;
         worldSpaceUnitsToolStripMenuItem.Checked = false;
         worldSpaceUnitToolStripMenuItem.Checked = true;
         offToolStripMenuItem.Checked = false;
      }

      private void worldSpaceUnitsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getRender().mbShowGrid = true;
         TerrainGlobals.getRender().mGridSpacing = 2.0f;
         worldSpaceUnitsToolStripMenuItem1.Checked = false;
         worldSpaceUnitsToolStripMenuItem.Checked = true;
         worldSpaceUnitToolStripMenuItem.Checked = false;
         offToolStripMenuItem.Checked = false;
      }

      private void worldSpaceUnitsToolStripMenuItem1_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getRender().mbShowGrid = true;
         TerrainGlobals.getRender().mGridSpacing = 4.0f;
         worldSpaceUnitsToolStripMenuItem1.Checked = true;
         worldSpaceUnitsToolStripMenuItem.Checked = false;
         worldSpaceUnitToolStripMenuItem.Checked = false;
         offToolStripMenuItem.Checked = false;
      }



      private void exportToR32ToolStripMenuItem_Click(object sender, EventArgs e)
      {
        

      }

      void exportHeights(HeightsGen.eHeightFileType type)
      {
         try
         {
            SaveFileDialog d = new SaveFileDialog();
            if (type == HeightsGen.eHeightFileType.cType_R32)
               {
                  d.Filter = "R32(*.R32)|*.R32";
               }
               else if (type == HeightsGen.eHeightFileType.cType_R16)
               {
                  d.Filter = "R16(*.R16)|*.R16|RAW 16bit(*.RAW)|*.RAW";
               }
               else if (type == HeightsGen.eHeightFileType.cType_R8)
               {
                  d.Filter = "RAW 8bit(*.RAW)|*.RAW";
               }
            
            d.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
            if (d.ShowDialog() == DialogResult.OK)
            {
               HeightsGen exp = new HeightsGen();
               exp.write_heights(d.FileName, type);
               exp.destroy();
               exp = null;
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         } 
      }
      private void r323ToolStripMenuItem_Click(object sender, EventArgs e)
      {
         exportHeights(HeightsGen.eHeightFileType.cType_R32);
      }

      private void rAW16BitIntToolStripMenuItem_Click(object sender, EventArgs e)
      {
         exportHeights(HeightsGen.eHeightFileType.cType_R16);
      }

      private void rAW8bitToolStripMenuItem_Click(object sender, EventArgs e)
      {
         exportHeights(HeightsGen.eHeightFileType.cType_R8);
      }

      private void calculateAOToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().computeAmbientOcclusion();
      }

      private void clearAOToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().clearAmbientOcclusion();
      }

      private void passabilityToMaskToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().maskFromSimPassibility();
      }

      private void artistModPanToolStripMenuItem_Click(object sender, EventArgs e)
      {
         //artistModPanToolStripMenuItem.Checked = !artistModPanToolStripMenuItem.Checked;

         CoreGlobals.getSettingsFile().ArtistModePan = artistModPanToolStripMenuItem.Checked;
         CoreGlobals.getSettingsFile().Save();
      }

      private void showHideFoliageToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().toggleViewFoliage();
      }

      private void xXToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getTerrainFrontEnd().mirrorTerrain( BTerrainFrontend.eMirrorTerrainType.eMirror_nXpX);
      }

      private void xXToolStripMenuItem1_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getTerrainFrontEnd().mirrorTerrain(BTerrainFrontend.eMirrorTerrainType.eMirror_pXnX);
      }

      private void yYToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getTerrainFrontEnd().mirrorTerrain(BTerrainFrontend.eMirrorTerrainType.eMirror_nYpY);
      }

      private void yYToolStripMenuItem1_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getTerrainFrontEnd().mirrorTerrain(BTerrainFrontend.eMirrorTerrainType.eMirror_pYnY);
      }


      private void allBlackToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getRender().mGridColor0 = new Vector4(0, 0, 0, 0.5f);
         TerrainGlobals.getRender().mGridColor1 = new Vector4(0, 0, 0, 0.5f);
         TerrainGlobals.getRender().mGridColor2 = new Vector4(0, 0, 0, 0.5f);
         TerrainGlobals.getRender().mGridColor3 = new Vector4(0, 0, 0, 0.5f);

         if (allBlackToolStripMenuItem.Checked)
         {
            whiteBlueToolStripMenuItem.Checked = false;
         }
      }

      private void whiteBlueToolStripMenuItem_Click(object sender, EventArgs e)
      {

         TerrainGlobals.getRender().mGridColor0 = new Vector4(1, 1, 1, 1);
         TerrainGlobals.getRender().mGridColor1 = new Vector4(1, 1, 1, 1);
         TerrainGlobals.getRender().mGridColor2 = new Vector4(1, 1, 1, 1);
         TerrainGlobals.getRender().mGridColor3 = new Vector4(0, 0, 1, 1);


         if (whiteBlueToolStripMenuItem.Checked)
         {
            allBlackToolStripMenuItem.Checked = false;
         }
      }

      private void timer1_Tick(object sender, EventArgs e)
      {
         if(CoreGlobals.getSettingsFile().AutoSaveEnabled)
         {
            XMBProcessor.Pause();
            string coreName = Path.GetFileNameWithoutExtension(CoreGlobals.ScenarioFile);
            
            if (CoreGlobals.getSettingsFile().AutoSaveToCustom || coreName =="")
            {
               doQuickSave("_autoSave");
            }
            else
            {
               
               doQuickSave(CoreGlobals.ScenarioDirectory,coreName);
            }
            XMBProcessor.Unpause();   
         }
      }

      private void autoSaveToolStripMenuItem_Click(object sender, EventArgs e)
      {
         AutoSaveDlg asd = new AutoSaveDlg();
         asd.ShowDialog();

         timer1.Enabled = CoreGlobals.getSettingsFile().AutoSaveEnabled;
         timer1.Interval = CoreGlobals.getSettingsFile().AutoSaveTimeInMinutes * 60000;
      }

      private void timer2_Tick(object sender, EventArgs e)
      {
         TerrainGlobals.getTerrainFrontEnd().updateMemoryEstimate(false,true);
      }

      private void superSimRepUpdateToolStripMenuItem_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().getSimRep().update(true,true);
      }

      private void randomGenToolStripMenuItem_Click(object sender, EventArgs e)
      {
         RandomMapGenerationForm rmgn = new RandomMapGenerationForm();
         PopupEditor editor = new PopupEditor();
         editor.ShowPopup(this, rmgn);
      }


   
   }


}

