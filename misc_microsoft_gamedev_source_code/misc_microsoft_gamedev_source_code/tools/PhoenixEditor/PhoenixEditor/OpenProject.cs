using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using System.IO;
using EditorCore;
using Terrain;

namespace PhoenixEditor
{
   public partial class OpenProject : Form
   {
      public bool mbDoOpenAs = false;
      public TerrainCreationParams mOpenAsParams = null;
      public bool mOpenAsCrop = true;

      public OpenProject()
      {
         InitializeComponent();
         PopulateFolderView();
         comboBox1.SelectedIndex = 0;
      }
      public OpenProject(bool bSelectionOnly)
      {
         InitializeComponent();
         PopulateFolderView();
         comboBox1.SelectedIndex = 0;

         if (bSelectionOnly == true)
         {
            this.Text = "Select a project to save over";
            //this.OKButton
         }
      }
      public string FileName = "";

      public void PopulateFolderView()
      {
         TreeNode root = new TreeNode("scenario");
//         root.Tag = CoreGlobals.getWorkPaths().mGameScenarioDirectory;
         BuildTree(root, CoreGlobals.getWorkPaths().mGameScenarioDirectory);

         DirectoryTreeView.Nodes.Add(root);


         root.Expand();
         DirectoryTreeView.SelectedNode = root;
      }

      public void BuildTree(TreeNode root, string path)
      {
         string[] dirs = Directory.GetDirectories(path);

         foreach (string subdir in dirs)
         {
            string[] files = Directory.GetFiles(subdir, "*.scn");
            if (files.Length == 0)
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
               
            }
         }


      }

      private void DirectoryTreeView_AfterSelect(object sender, TreeViewEventArgs e)
      {
         if (e.Node.Tag != null)
         {
            this.textBox1.Text = e.Node.Tag.ToString();
            FileName = e.Node.Tag.ToString();
            comboBox1.SelectedIndex = 0;
         }
      }

      private void OKButton_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.Close();

      }

      private void DirectoryTreeView_DoubleClick(object sender, EventArgs e)
      {

         if (DirectoryTreeView.SelectedNode.Tag != null)
         {
            this.textBox1.Text = DirectoryTreeView.SelectedNode.Tag.ToString();
            FileName = DirectoryTreeView.SelectedNode.Tag.ToString();

            comboBox1.SelectedIndex = 0;
            DialogResult = DialogResult.OK;
            this.Close();
         }

      }

      private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         if(comboBox1.SelectedIndex == 0)
         {
            radioButton1.Enabled = false;
            radioButton2.Enabled = false;
            mbDoOpenAs = false;
            mOpenAsParams = null;
         }
         else
         {
            mOpenAsParams = new TerrainCreationParams();
            

            radioButton1.Enabled = true;
            radioButton2.Enabled = true;
            mbDoOpenAs = true;

            if (comboBox1.SelectedIndex == 1) mOpenAsParams.initFromVisData((int)eMapSizes.cMap_Small_512v, (int)eMapSizes.cMap_Small_512v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min); 
            if (comboBox1.SelectedIndex == 2) mOpenAsParams.initFromVisData((int)eMapSizes.cMap_Small_640v, (int)eMapSizes.cMap_Small_640v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);  
            if (comboBox1.SelectedIndex == 3) mOpenAsParams.initFromVisData((int)eMapSizes.cMap_Small_768v, (int)eMapSizes.cMap_Small_768v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);  
            if (comboBox1.SelectedIndex == 4) mOpenAsParams.initFromVisData((int)eMapSizes.cMap_Small_896v, (int)eMapSizes.cMap_Small_896v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min);  
            if (comboBox1.SelectedIndex == 5) mOpenAsParams.initFromVisData((int)eMapSizes.cMap_Large_1024v, (int)eMapSizes.cMap_Large_1024v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min); ;
            if (comboBox1.SelectedIndex == 6) mOpenAsParams.initFromVisData((int)eMapSizes.cMap_Large_1280v, (int)eMapSizes.cMap_Large_1280v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min); ;
            if (comboBox1.SelectedIndex == 7) mOpenAsParams.initFromVisData((int)eMapSizes.cMap_Large_1536v, (int)eMapSizes.cMap_Large_1536v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min); ;
            if (comboBox1.SelectedIndex == 8) mOpenAsParams.initFromVisData((int)eMapSizes.cMap_Large_1792v, (int)eMapSizes.cMap_Large_1792v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min); ;
            if (comboBox1.SelectedIndex == 9) mOpenAsParams.initFromVisData((int)eMapSizes.cMap_Large_2048v, (int)eMapSizes.cMap_Large_2048v, TerrainCreationParams.cTileSpacingOne, (float)eSimQuality.cQuality_Min); ;

         }

      }

      public bool mbOnlyForSave = false;
      private void OpenProject_Load(object sender, EventArgs e)
      {
         if(mbOnlyForSave)
         {
            comboBox1.SelectedIndex=0;
            comboBox1.Enabled = false;
         }
      }

      private void radioButton1_CheckedChanged(object sender, EventArgs e)
      {
         mOpenAsCrop = true;
      }

      private void radioButton2_CheckedChanged(object sender, EventArgs e)
      {
         mOpenAsCrop = false;
      }

   }
}