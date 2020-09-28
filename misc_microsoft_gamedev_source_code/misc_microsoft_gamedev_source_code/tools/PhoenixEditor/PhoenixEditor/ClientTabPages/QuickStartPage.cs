using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

using PhoenixEditor;
using EditorCore;
using Terrain;

namespace PhoenixEditor.ClientTabPages
{
   public partial class QuickStartPage : BaseClientPage
   {
      public QuickStartPage()
      {
         InitializeComponent();
         //OpenItems(CoreGlobals.getWorkPaths().mGameScenarioDirectory);

         try
         {
            TreeNode root = new TreeNode("scenario");
            BuildTree(root, CoreGlobals.getWorkPaths().mGameScenarioDirectory);
            ScenarioTreeView.Nodes.Add(root);
            root.Expand();

            string background = Path.Combine(CoreGlobals.getWorkPaths().mAppIconDir, "startpage.jpg");
            if (File.Exists(background))
            {
               this.panel1.BackgroundImage = Image.FromFile(background);
               this.panel1.BackgroundImageLayout = ImageLayout.Center;
            }

            string buildnotefile = Path.Combine(CoreGlobals.getWorkPaths().mBaseDirectory, "buildnotes.txt");
            if (File.Exists(buildnotefile))
            {
               this.textBox1.Text = File.ReadAllText(buildnotefile);

            }
            this.textBox1.ScrollBars = ScrollBars.Both;
         }
         catch(System.Exception ex)
         {
            ex.ToString();
         }

      }
      public override void Activate()
      {
         ScenarioTreeView.Nodes.Clear();
         TreeNode root = new TreeNode("scenario");
         BuildTree(root, CoreGlobals.getWorkPaths().mGameScenarioDirectory);
         ScenarioTreeView.Nodes.Add(root);
         root.Expand();
      }
      override public void destroyPage()
      {
         base.destroyPage();
      }

      public void OpenItems(string path)
      {
         string[] dirs = Directory.GetDirectories(path);

         foreach (string subdir in dirs)
         {
            string[] files = Directory.GetFiles(subdir, "*.scn");
            if (files.Length != 0)
            {
               Xceed.SmartUI.Controls.OfficeTaskPane.Task newTask = new Xceed.SmartUI.Controls.OfficeTaskPane.Task();
               string name = subdir.Remove(0, path.Length + 1);         
               newTask.Text = name;
               newTask.Tag = files[0];
               newTask.Image = global::PhoenixEditor.Properties.Resources.phx16;
               newTask.Click+=new Xceed.SmartUI.SmartItemClickEventHandler(OpenTask_Click);
               OpenGroup.Items.Add(newTask);
            }

         }
      }

      public void BuildTree(TreeNode root, string path)
      {
         string[] dirs = Directory.GetDirectories(path);

         foreach (string subdir in dirs)
         {
            string[] files = Directory.GetFiles(subdir, "*.scn");
            if (files.Length == 0 )
            {
               if (Directory.GetFiles(subdir, "*.scn", SearchOption.AllDirectories).Length > 0)
               {
                  //parent folder
                  TreeNode subNode = new TreeNode(subdir.Remove(0, path.Length + 1));
                  root.Nodes.Add(subNode);
                  //subNode.Tag = subdir;
                  subNode.ImageIndex = 0;
                  subNode.SelectedImageIndex = 0;

                  //not recursive for now...
                  BuildTree(subNode, subdir);
               }
            }
            else
            {
                 
               //project folder
               TreeNode subNode = new TreeNode(subdir.Remove(0, path.Length + 1));
               subNode.Tag = files[0];//subdir;
               subNode.ImageIndex = 1;
               subNode.SelectedImageIndex = 1;
               root.Nodes.Add(subNode);
               BuildTree(subNode, subdir);

               if (files[0].Contains("artists_default"))
               {
                  mDefaultTinyMap = files[0];
               }
               
            }
         }


      }

      string mDefaultTinyMap = "";


      void OpenTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         Xceed.SmartUI.Controls.OfficeTaskPane.Task task = sender as Xceed.SmartUI.Controls.OfficeTaskPane.Task;

         MainWindow.mMainWindow.OpenScenario(task.Tag.ToString());
      }
      private void ArtistsDefaultTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         if (mDefaultTinyMap != "")
         {
            MainWindow.mMainWindow.OpenScenario(mDefaultTinyMap);
         }
         else
         {
            CoreGlobals.ShowMessage("Artists_default map not found.  please get latest");
         }
      }

      private void newMicro_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Tiny_128v, (int)eMapSizes.cMap_Tiny_128v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);
      }

      private void newTiny_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      { 
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Tiny_256v, (int)eMapSizes.cMap_Tiny_256v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);
      }
      private void task21_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Small_512v, (int)eMapSizes.cMap_Small_512v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);
      }

      private void task20_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Small_640v, (int)eMapSizes.cMap_Small_640v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);
      }

      private void task19_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Small_768v, (int)eMapSizes.cMap_Small_768v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);
      }

      private void task18_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Small_896v, (int)eMapSizes.cMap_Small_896v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);
      }
      private void newSmall_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Large_1024v, (int)eMapSizes.cMap_Large_1024v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);

      }
      private void newSmallMed_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Large_1280v, (int)eMapSizes.cMap_Large_1280v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);
      }
      private void newMedium_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Large_1536v, (int)eMapSizes.cMap_Large_1536v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);
      }

      private void newMedLarge_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Large_1792v, (int)eMapSizes.cMap_Large_1792v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);
      }

      private void newLarge_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         TerrainCreationParams param = new TerrainCreationParams();
         param.initFromVisData((int)eMapSizes.cMap_Large_2048v, (int)eMapSizes.cMap_Large_2048v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);
         MainWindow.mMainWindow.NewScenario(param);

      }
     
      private void task12_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         //NewScenario mNewScenarioDlg = new NewScenario();
         //if (mNewScenarioDlg.ShowDialog() == DialogResult.OK)
         //{
         //   MainWindow.mMainWindow.NewScenario(mNewScenarioDlg.mParams);
         //}
      }

      private void smartOfficeTaskPane1_Paint(object sender, PaintEventArgs e)
      {
         
      }

      private void NewParticleEdTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.NewParticleEditor(null);
      }

      private void NewVisualEditorTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.NewVisualEditor(null);
      }

      private void task9_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.OpenTerrainTypesEditor();
      }

      private void smartOfficeTaskPane1_Click(object sender, EventArgs e)
      {

      }

      private void task10_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.ShowGameDatabase();

      }

      private void PowersTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.NewPowersEditor();

      }


      private void task13_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         BatchExporter be = new BatchExporter();
         be.mainInit(null);
         be.Show();
         be = null;
      }

      private void ScenarioDataTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.NewScenarioDataEditor();

      }

      private void TriggerDefinitionTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.NewTriggerDataDefinitionEditor();

      }

      private void AbilitiesTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.NewAbilityEditor();
      }

      private void OtherScriptsTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.NewScriptEditor();

      }

      private void TriggerTemplateTask2_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.NewTriggerTemplateEditor();

      }

      private void ScenarioTreeView_AfterSelect(object sender, TreeViewEventArgs e)
      {
         if (e.Node.Tag == null)
            return; 
         Cursor.Current = Cursors.WaitCursor;
         MainWindow.mMainWindow.OpenScenario(e.Node.Tag.ToString());
         Cursor.Current = Cursors.Arrow;
      }

      private void ControllerConfigurationTask_Click( object sender, Xceed.SmartUI.SmartItemClickEventArgs e )
      {
         MainWindow.mMainWindow.NewControllerConfigurationsEditor();
      }

      private void StringTableTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         MainWindow.mMainWindow.NewStringTableEditor();

      }

      private void ObjectEditorTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         //if (!CoreGlobals.IsDev)
         //{
         //   CoreGlobals.ShowMessage("not ready yet!");
         //}
         //else
         {

            MainWindow.mMainWindow.NewObjectEditor();
         }
      }

      private void HintDatabaseTask_Click(object sender, Xceed.SmartUI.SmartItemClickEventArgs e)
      {
         //if (CoreGlobals.IsDev == true)
         {
            MainWindow.mMainWindow.NewHintDatabaseEditor();
         }
      }



      
   
   }
}
