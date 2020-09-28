using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using System.IO;

using EditorCore;

namespace PhoenixEditor
{
   public partial class SaveProject : Form
   {
      public SaveProject()
      {
         InitializeComponent();
         PopulateFolderView();

         //this.label1.Text = CoreGlobals.getWorkPaths().mGameScenarioDirectory;

         mBaseDir = CoreGlobals.getWorkPaths().mGameScenarioDirectory;
      }
      private string mBaseDir;
      public string ProjectPath = "";
      public string FileName = "";

      public void PopulateFolderView()
      {


         TreeNode root = new TreeNode("scenario");
         root.Tag = CoreGlobals.getWorkPaths().mGameScenarioDirectory;
         BuildTree(root, CoreGlobals.getWorkPaths().mGameScenarioDirectory);

         DirectoryTreeView.Nodes.Add(root);


         root.Expand();
         DirectoryTreeView.SelectedNode = root;
      }
      /*
       *   string[] dirs = Directory.GetDirectories(path);

         foreach (string subdir in dirs)
         {
            string[] files = Directory.GetFiles(subdir, "*.scn");
            if (files.Length == 0)
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
       */
      public int BuildTree(TreeNode root, string path)
      {
         string[] dirs = Directory.GetDirectories(path);

         int children = 0;
         int leafs = 0;
         foreach (string subdir in dirs)
         {
            if (Directory.GetFiles(subdir, "*.scn").Length == 0)
            {
               if (Directory.GetFiles(subdir, "*.gls").Length != 0)
               {
                  continue;
               }

               //parent folder
               TreeNode subNode = new TreeNode(subdir.Remove(0, path.Length + 1));
               subNode.Tag = subdir;
               subNode.ImageIndex = 0;
               subNode.SelectedImageIndex = 0;

               leafs = BuildTree(subNode, subdir);
               children += leafs;

               if (leafs > 0)  //This line cleans up the selection, but make it unclear how to add a new folder
               {
                  root.Nodes.Add(subNode);
               }

            }
            else
            {
               children++;
               //do we have subfolders?
               //string[] subdirs = Directory.GetDirectories(subdir);
               //if (subdirs.Length != 0)
               //{
               //   for(int i=0;i<subdirs.Length;i++)
               //   {
               //      //project folder
               //      TreeNode subNode = new TreeNode(subdir.Remove(0, path.Length + 1));
               //      subNode.Tag = subdirs[i];//subdir;
               //      subNode.ImageIndex = 0;
               //      subNode.SelectedImageIndex = 0;
               //      root.Nodes.Add(subNode);

               //      BuildTree(subNode, subdirs[i]);
               //   }
               //}

               ////project folder
               //TreeNode subNode = new TreeNode(subdir.Remove(0, path.Length + 1));
               //subNode.ImageIndex = 1;
               //subNode.SelectedImageIndex = 1;
               

               //root.Nodes.Add(subNode);

            }
         }
         return children;

      }
      string mSubdirectory = "";
      private void DirectoryTreeView_AfterSelect(object sender, TreeViewEventArgs e)
      {
         if (e.Node.Tag != null)
         {
            //this.label1.Text = "Path: " + e.Node.Tag.ToString();

            mSubdirectory = e.Node.Tag.ToString();
            mSubdirectory = mSubdirectory.Replace(CoreGlobals.getWorkPaths().mGameScenarioDirectory, "");

            if (ProjectNameTextBox.Text != "")
            {

               string newPath = Path.Combine(mBaseDir + mSubdirectory, ProjectNameTextBox.Text) + "\\" + Path.GetFileNameWithoutExtension(ProjectNameTextBox.Text) + ".scn";
               this.label1.Text = newPath;
            }
         }
      }

      private void OKButton_Click(object sender, EventArgs e)
      {
         string newDir = Path.Combine(mBaseDir + mSubdirectory, ProjectNameTextBox.Text);
         
         if (ProjectNameTextBox.Text == "")
         {
            MessageBox.Show("You must type in a project name");
            this.DialogResult = DialogResult.None;
            return;
         }
         if (ProjectNameTextBox.Text.Contains(" "))
         {
            MessageBox.Show("Project name cannot contain spaces.  Use underscore instead: _");
            this.DialogResult = DialogResult.None;

            return;
         }

         if(Directory.Exists(newDir))
         {
            if(MessageBox.Show("Project already exists.  Overwrite files?","Warning",MessageBoxButtons.OKCancel) == DialogResult.OK)
            {
            }
            else
            {
               this.DialogResult = DialogResult.None;
               //this.DialogResult = DialogResult.Cancel;
               return;
            }
         }






         try
         {
            Directory.CreateDirectory(newDir);
         }
         catch(System.Exception ex)
         {
            MessageBox.Show(ex.Message);
            this.DialogResult = DialogResult.Cancel;
            return;
         }
         ProjectPath = newDir;
         FileName = Path.Combine(newDir, Path.GetFileNameWithoutExtension(ProjectNameTextBox.Text) + ".scn" );
         this.DialogResult = DialogResult.OK;
         this.Close();
      }

      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      private void ProjectNameTextBox_TextChanged(object sender, EventArgs e)
      {
         //string newPath = Path.Combine(mBaseDir, ProjectNameTextBox.Text) + "\\" + Path.GetFileNameWithoutExtension(ProjectNameTextBox.Text) + ".scn";
         string newPath = Path.Combine(mBaseDir + mSubdirectory, ProjectNameTextBox.Text) + "\\" + Path.GetFileNameWithoutExtension(ProjectNameTextBox.Text) + ".scn";
         this.label1.Text = newPath;
      }

      private void BrowseButton_Click(object sender, EventArgs e)
      {
         OpenProject d = new OpenProject(true);
         d.mbOnlyForSave = true;
         if(d.ShowDialog() == DialogResult.OK)
         {
            ProjectNameTextBox.Text = Path.GetFileNameWithoutExtension(d.FileName);

            string dir = Path.GetDirectoryName(Path.GetDirectoryName(d.FileName));
            mSubdirectory = dir.Replace(CoreGlobals.getWorkPaths().mGameScenarioDirectory, "");

            if (ProjectNameTextBox.Text != "")
            {
               string newPath = Path.Combine(mBaseDir + mSubdirectory, ProjectNameTextBox.Text) + "\\" + Path.GetFileNameWithoutExtension(ProjectNameTextBox.Text) + ".scn";
               this.label1.Text = newPath;
            }
         }
      }

      private void SaveProject_Load(object sender, EventArgs e)
      {

      }

      private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
      {
         MessageBox.Show("Example to create the new folder \"Final\" under \"Scenario\\Skirmish\" do this: \n\r  1.  Click the Skirmish folder in the tree.   2.  Type \"Final\\\" before your project name (as in \"Final\\havest2v2_swi\" ");

      }
   }
}