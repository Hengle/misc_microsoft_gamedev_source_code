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
   public partial class ComponentAssetModelPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualModelComponentAsset mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;

      public ComponentAssetModelPage()
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
         fileBrowseControl2.FileName = mData.damagefile;
         sizeXFloatSliderEdit.Value = mData.uvOffset[0].uOfs;
         sizeZFloatSliderEdit.Value = mData.uvOffset[0].vOfs;
         floatSliderEdit2.Value = mData.uvOffset[1].uOfs;
         floatSliderEdit3.Value = mData.uvOffset[1].vOfs;
         floatSliderEdit4.Value = mData.uvOffset[2].uOfs;
         floatSliderEdit5.Value = mData.uvOffset[2].vOfs;
         
         mIsBindingData = false;
      }

      public void updateData()
      {
         if ((mData == null) || (mNode == null))
            return;

         // Move control data to data (CONTROL DATA -> DATA)
         //
         visualModelComponentAsset afterChanges = new visualModelComponentAsset();
         afterChanges.type = visualModelComponentAsset.ComponentAssetType.Model;
         afterChanges.file = fileBrowseControl1.FileName;
         afterChanges.damagefile = fileBrowseControl2.FileName;

         afterChanges.uvOffset[0].uOfs = sizeXFloatSliderEdit.Value;
         afterChanges.uvOffset[0].vOfs = sizeZFloatSliderEdit.Value;
         afterChanges.uvOffset[1].uOfs = floatSliderEdit2.Value;
         afterChanges.uvOffset[1].vOfs = floatSliderEdit3.Value;
         afterChanges.uvOffset[2].uOfs = floatSliderEdit4.Value;
         afterChanges.uvOffset[2].vOfs = floatSliderEdit5.Value;

         // Load asset
         afterChanges.loadAsset();

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

         // Load new model file
         mData.loadAsset();
      }

      private void fileBrowseControl2_ValueChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void sizeXFloatSliderEdit_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void sizeZFloatSliderEdit_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void floatSliderEdit2_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void floatSliderEdit3_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void floatSliderEdit4_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void floatSliderEdit5_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }
   }
}
