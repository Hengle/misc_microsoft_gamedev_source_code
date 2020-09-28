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
   public partial class AnimTagCameraShakePage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualModelAnimAssetTag mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;

      public AnimTagCameraShakePage()
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
         forceSliderEdit.Value = (float)mData.force;
         durationSliderEdit.Value = (float)mData.lifespan;
         selectedCheckBox.Checked = (bool)mData.checkSelected;


         bool found = false;
         for (int i = 0; i < toBoneComboBox.Items.Count; i++)
         {
            if (String.Compare(mData.tobone, (string)toBoneComboBox.Items[i], true) == 0)
            {
               toBoneComboBox.SelectedIndex = i;
               found = true;
               break;
            }
         }

         if (!found)
         {
            toBoneComboBox.SelectedIndex = -1;
            toBoneComboBox.SelectedItem = null;
            toBoneComboBox.Text = null;
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
         afterChanges.force = (decimal)forceSliderEdit.Value;
         afterChanges.tobone = (string)toBoneComboBox.SelectedItem;
         afterChanges.lifespan = (decimal)durationSliderEdit.Value;
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
         toBoneComboBox.Items.Clear();

         // Get model we are attaching to
         TreeNode animAssetNode = mNode.Parent;
         TreeNode animNode = animAssetNode.Parent;
         TreeNode modelNode = animNode.Parent;

         visualModel vmodel = modelNode.Tag as visualModel;
         if ((vmodel != null) && (vmodel.component != null))
         {
            GrannyInstance granInst = null;

            if (vmodel.component.asset != null)
            {
               granInst = vmodel.component.asset.mInstance;
            }
            else if ((vmodel.component.logic != null) &&
                     (vmodel.component.logic.type == visualLogic.LogicType.Variation) &&
                     (vmodel.component.logic.logicdata.Count > 0))
            {
               visualLogicData curLogicData = vmodel.component.logic.logicdata[0];
               if (curLogicData.asset != null)
               {
                  granInst = curLogicData.asset.mInstance;
               }
            }

            if (granInst != null)
            {
               List<string> boneList = new List<string>();
               boneList.Clear();
               granInst.getBoneNames(ref boneList);

               // check all tags
               foreach (string curBoneName in boneList)
               {
                  if (!String.IsNullOrEmpty(curBoneName))
                     toBoneComboBox.Items.Add(curBoneName);
               }
            }
         }
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

      private void toBoneComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
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
