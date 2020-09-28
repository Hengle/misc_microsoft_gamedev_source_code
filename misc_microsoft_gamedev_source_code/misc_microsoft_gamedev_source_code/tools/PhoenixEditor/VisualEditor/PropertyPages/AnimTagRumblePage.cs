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
   public partial class AnimTagRumblePage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualModelAnimAssetTag mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;

      public AnimTagRumblePage()
      {
         InitializeComponent();
      }



      public void setVisualEditorPage(VisualEditorPage page)
      {
         mVisualEditorPage = page;
      }

      public void bindData(visualModelAnimAssetTag model, TreeNode treeNode)
      {
         mIsBindingData = true;

         mData = model;
         mNode = treeNode;

         // Move data to control data (DATA -> CONTROL DATA)
         //
         refreshComboBoxItems();

         positionSliderEdit.Value = (float)mData.position;
         leftStrengthSliderEdit.Value = (float)mData.force;
         rightStrengthSliderEdit.Value = (float)mData.force2;
         durationSliderEdit.Value = (float)mData.lifespan;
         loopCheckBox.Checked = (bool)mData.loop;
         selectedCheckBox.Checked = (bool)mData.checkSelected;
 
         bool found = false;
         for (int i = 0; i < leftRumbleComboBox.Items.Count; i++)
         {
            if (String.Compare(mData.leftRumbleType, (string)leftRumbleComboBox.Items[i], true) == 0)
            {
               leftRumbleComboBox.SelectedIndex = i;
               found = true;
               break;
            }
         }
         if (!found)
         {
            leftRumbleComboBox.SelectedIndex = -1;
            leftRumbleComboBox.SelectedItem = null;
            leftRumbleComboBox.Text = null;
         }

         found = false;
         for (int i = 0; i < rightRumbleComboBox.Items.Count; i++)
         {
            if (String.Compare(mData.rightRumbleType, (string)rightRumbleComboBox.Items[i], true) == 0)
            {
               rightRumbleComboBox.SelectedIndex = i;
               found = true;
               break;
            }
         }
         if (!found)
         {
            rightRumbleComboBox.SelectedIndex = -1;
            rightRumbleComboBox.SelectedItem = null;
            rightRumbleComboBox.Text = null;
         }

         mIsBindingData = false;
      }

      public void updateData()
      {
         if ((mData == null) || (mNode == null))
            return;

         // Move control data to data (CONTROL DATA -> DATA)
         //
         visualModelAnimAssetTag afterChanges = new visualModelAnimAssetTag();
         afterChanges.copy(mData);
         afterChanges.position = (decimal)positionSliderEdit.Value;
         afterChanges.force = (decimal)leftStrengthSliderEdit.Value;
         afterChanges.force2 = (decimal)rightStrengthSliderEdit.Value;
         afterChanges.leftRumbleType = (string)leftRumbleComboBox.SelectedItem;
         afterChanges.rightRumbleType = (string)rightRumbleComboBox.SelectedItem;
         afterChanges.lifespan = (decimal)durationSliderEdit.Value;
         afterChanges.loop = (bool)loopCheckBox.Checked;
         afterChanges.checkSelected = (bool)selectedCheckBox.Checked;

         // Add/Execute undo action
         UndoRedoChangeDataAction undoAction = new UndoRedoChangeDataAction(mData, afterChanges);
         mVisualEditorPage.mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);


         // Update animation slider
         mVisualEditorPage.setAnimationControlState(AnimationControl.AnimControlStateEnum.ePaused);
         mVisualEditorPage.setAnimationControlNormalizedTime((float)afterChanges.position);
      }

      public void refreshComboBoxItems()
      {
         leftRumbleComboBox.Items.Clear();
         leftRumbleComboBox.Items.Add("None");
         leftRumbleComboBox.Items.Add("Fixed");
         leftRumbleComboBox.Items.Add("SineWave");
         leftRumbleComboBox.Items.Add("IntervalBurst");
         leftRumbleComboBox.Items.Add("RandomNoise");
         leftRumbleComboBox.Items.Add("Incline");
         leftRumbleComboBox.Items.Add("Decline");

         rightRumbleComboBox.Items.Clear();
         rightRumbleComboBox.Items.Add("None");
         rightRumbleComboBox.Items.Add("Fixed");
         rightRumbleComboBox.Items.Add("SineWave");
         rightRumbleComboBox.Items.Add("IntervalBurst");
         rightRumbleComboBox.Items.Add("RandomNoise");
         rightRumbleComboBox.Items.Add("Incline");
         rightRumbleComboBox.Items.Add("Decline");
      }


      // ---------------------------------
      // Even Handlers
      // ---------------------------------
      private void RefreshButton_Click(object sender, EventArgs e)
      {
         if (mVisualEditorPage.getAnimationControlState() == AnimationControl.AnimControlStateEnum.ePaused)
         {
            float curControlValue = mVisualEditorPage.getAnimationControlNormalizedTime();

            if (positionSliderEdit.Value != curControlValue)
            {
               positionSliderEdit.Value = curControlValue;
               updateData();
            }
         }
      }

      private void forceSliderEdit_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void positionSliderEdit_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void durationSliderEdit_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void rightRumbleComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void loopCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void leftRumbleComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void label4_Click(object sender, EventArgs e)
      {

      }

      private void selectedCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }
   }
}
