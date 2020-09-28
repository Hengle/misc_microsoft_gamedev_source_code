using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using ModelSystem;


namespace VisualEditor.PropertyPages
{
   public partial class ComponentAssetLightPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualModelComponentAsset mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;

      public ComponentAssetLightPage()
      {
         InitializeComponent();
      }

      public void setVisualEditorPage(VisualEditorPage page)
      {
         mVisualEditorPage = page;
      }

      public void bindData(visualModelComponentAsset componentAsset, TreeNode treeNode)
      {
         mIsBindingData = true;

         mData = componentAsset;
         mNode = treeNode;

         // Move data to control data (DATA -> CONTROL DATA)
         //
         fileBrowseControl1.FileName = mData.file;

         mIsBindingData = false;
      }

      public void updateData()
      {
         if ((mData == null) || (mNode == null))
            return;

         // Move control data to data (CONTROL DATA -> DATA)
         //
         visualModelComponentAsset afterChanges = new visualModelComponentAsset();
         afterChanges.type = visualModelComponentAsset.ComponentAssetType.Light;
         afterChanges.file = fileBrowseControl1.FileName;

         // Add/Execute undo action
         UndoRedoChangeDataAction undoAction = new UndoRedoChangeDataAction(mData, afterChanges);
         mVisualEditorPage.mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
      }


      // ---------------------------------
      // Even Handlers
      // ---------------------------------

      private void fileBrowseControl1_ValueChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }
   }
}
