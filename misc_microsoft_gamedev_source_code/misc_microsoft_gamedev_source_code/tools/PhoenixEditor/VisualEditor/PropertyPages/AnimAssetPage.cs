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
   public partial class AnimAssetPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualModelAnimAsset mData = null;
      private TreeNode mNode = null;
      private EditorCore.FloatProgression mProgression = null;
      private bool mIsBindingData = false;

      public AnimAssetPage()
      {
         InitializeComponent();
      }

      public void setVisualEditorPage(VisualEditorPage page)
      {
         mVisualEditorPage = page;
      }

      public void bindData(visualModelAnimAsset animAsset, TreeNode treeNode)
      {
         mIsBindingData = true;

         mData = animAsset;
         mNode = treeNode;

         // Move data to control data (DATA -> CONTROL DATA)
         //
         fileBrowseControl1.FileName = mData.file;
         weightNumericUpDown1.Value = mData.weight;

         if (mData.progression != null)
         {
            opacityProgressionCheckBox.Checked = true;
            opacityProgressionButton.Enabled = true;

            mProgression = new EditorCore.FloatProgression();
            mProgression.Copy(mData.progression);
         }
         else
         {
            opacityProgressionCheckBox.Checked = false;
            opacityProgressionButton.Enabled = false;

            mProgression = null;
         }

         mIsBindingData = false;
      }

      public void updateData()
      {
         if ((mData == null) || (mNode == null))
            return;

         // Move control data to data (CONTROL DATA -> DATA)
         //
         visualModelAnimAsset afterChanges = new visualModelAnimAsset();
         afterChanges.file = fileBrowseControl1.FileName;
         afterChanges.weight = (int)weightNumericUpDown1.Value;

         // Create a progression copy if:
         //    - progression is not null
         //    - progression has 2 or more points
         //    - if progression only has 2 points makes sure they are not the default values
         if ((mProgression != null) && 
             (mProgression.Stages.Count >= 2))
             //((mProgression.Stages.Count == 2) && (mProgression.Stages[0].Value != 1.0f) || (mProgression.Stages[1].Value != 1.0f))
         {
            afterChanges.progression = new EditorCore.FloatProgression();
            afterChanges.progression.Copy(mProgression);
         }
         else
         {
            afterChanges.progression = null;
         }

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

         // Must reapply the animation now
         mVisualEditorPage.ShowPropertyPageForSelectedNode();
      }

      private void weightNumericUpDown1_ValueChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void opacityProgressionCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;


         if (opacityProgressionCheckBox.Checked)
         {
            opacityProgressionButton.Enabled = true;
            mProgression = new EditorCore.FloatProgression();

            mProgression.addStage(0.5f, 0, 0);
            mProgression.addStage(0.5f, 0, 1);
         }
         else
         {
            opacityProgressionButton.Enabled = false;
            mProgression = null;
         }


         updateData();
      }

      private void opacityProgressionButton_Click(object sender, EventArgs e)
      {
         Dialogs.OpacityProgressionDialog d = new Dialogs.OpacityProgressionDialog();

         EditorCore.FloatProgression progression = new EditorCore.FloatProgression();

         progression.Copy(mProgression);
         progression.Scale(100.0f);

         d.setData(progression);

         if (d.ShowDialog() == DialogResult.OK)
         {
            progression.Scale(0.01f);

            // Check for differences
            if (!mProgression.IsEqual(progression))
            {
               mProgression.Copy(progression);
               updateData();
            }
         }
      }

   }
}
