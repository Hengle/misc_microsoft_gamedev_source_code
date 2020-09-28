using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace EditorCore
{
   public partial class SubfolderPicker : UserControl
   {
      public SubfolderPicker()
      {
         InitializeComponent();
      }

      
      string mRootFolder = "";
      
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public string RootFolder
      {
         get
         {
            return mRootFolder;
         }
         set
         {
            mRootFolder = value;

            if(mRootFolder[mRootFolder.Length-1] == '\\')
            {
               mRootFolder = mRootFolder.Substring(0, mRootFolder.Length - 1);
            }

            TreeNode root = new TreeNode(mRootFolder.Remove(0, mRootFolder.LastIndexOf('\\') + 1));
            root.Tag = mRootFolder;
            root.ImageIndex = 0;
            root.SelectedImageIndex = 0;
            treeView1.Nodes.Add(root);

            BuildTree(root, mRootFolder);
            treeView1.Nodes.Clear();
            treeView1.Nodes.Add(root);
            root.Expand();
         }


      }

      private void BuildTree(TreeNode root, string path)
      {
         string[] dirs = Directory.GetDirectories(path);

         foreach (string subdir in dirs)
         {
            TreeNode subNode = new TreeNode(subdir.Remove(0, path.Length + 1));
            subNode.Tag = subdir;
            subNode.ImageIndex = 0;
            subNode.SelectedImageIndex = 0;
            root.Nodes.Add(subNode);

            BuildTree(subNode, subdir);
         }
      }

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public string SelectedFolder
      {
         get
         {
            if(mSelectedNode == null)
               return "";
            return mSelectedNode.Text;

         }         
      }
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public string SelectedPath
      {
         get
         {
            if (mSelectedNode == null)
               return mRootFolder;

            return mSelectedNode.Tag.ToString();

         }
      }
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public string RelativePath
      {
         get
         {
            if (mSelectedNode == null)
               return "";
            if(mSelectedNode.Tag.ToString() == mRootFolder)
            {
               return "";
            }
            return mSelectedNode.Tag.ToString().Remove(0, mRootFolder.Length + 1);
         }
      }

      public event EventHandler FolderSelected;

      private void treeView1_Click(object sender, EventArgs e)
      {

      }
      TreeNode mSelectedNode = null;
      private void treeView1_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
      {
         mSelectedNode = e.Node;

         if(FolderSelected != null)
         {
            FolderSelected(this, null);
         }
      }


   }
}
