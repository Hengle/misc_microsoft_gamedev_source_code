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
   public partial class AttachModelRefPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualModelComponentOrAnimAttach mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;


      public AttachModelRefPage()
      {
         InitializeComponent();
      }

      public void setVisualEditorPage(VisualEditorPage page)
      {
         mVisualEditorPage = page;
      }

      public void bindData(visualModelComponentOrAnimAttach attach, TreeNode treeNode)
      {
         mIsBindingData = true;

         mData = attach;
         mNode = treeNode;


         // Move data to control data (DATA -> CONTROL DATA)
         //
         refreshComboBoxItems();


         syncAnimsCheckBox.Checked = mData.syncanims;
         disregardBoneOrientationCheckBox.Checked = mData.disregardorient;

         bool found3 = false;
         for (int i = 0; i < modelRefComboBox.Items.Count; i++)
         {
            if (String.Compare(mData.name, (string)modelRefComboBox.Items[i], true) == 0)
            {
               modelRefComboBox.SelectedIndex = i;
               found3 = true;
               break;
            }
         }

         if (!found3)
         {
            modelRefComboBox.SelectedIndex = -1;
            modelRefComboBox.SelectedItem = null;
            modelRefComboBox.Text = null;
         }


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


         bool found2 = false;
         for (int i = 0; i < fromBoneComboBox.Items.Count; i++)
         {
            if (String.Compare(mData.frombone, (string)fromBoneComboBox.Items[i], true) == 0)
            {
               fromBoneComboBox.SelectedIndex = i;
               found2 = true;
               break;
            }
         }

         if (!found2)
         {
            fromBoneComboBox.SelectedIndex = -1;
            fromBoneComboBox.SelectedItem = null;
            fromBoneComboBox.Text = null;
         }

         mIsBindingData = false;
      }

      public void updateData()
      {
         if ((mData == null) || (mNode == null))
            return;

         // Move control data to data (CONTROL DATA -> DATA)
         //
         visualModelComponentOrAnimAttach afterChanges = new visualModelComponentOrAnimAttach();
         afterChanges.type = visualModelComponentOrAnimAttach.AttachType.ModelRef;
         afterChanges.syncanims = syncAnimsCheckBox.Checked;
         afterChanges.name = (string)modelRefComboBox.SelectedItem;
         afterChanges.tobone = (string)toBoneComboBox.SelectedItem;
         afterChanges.frombone = (string)fromBoneComboBox.SelectedItem;
         afterChanges.disregardorient = disregardBoneOrientationCheckBox.Checked;

         // Add/Execute undo action
         UndoRedoChangeDataAction undoAction = new UndoRedoChangeDataAction(mData, afterChanges);
         mVisualEditorPage.mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
      }


      public void refreshComboBoxItems()
      {
         toBoneComboBox.Items.Clear();

         // Get model we are attaching to
         TreeNode animNode = mNode.Parent;
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


         ModelSystem.GrannyInstance instance = null;
         fromBoneComboBox.Items.Clear();

         // If dealing with a modelref get get instance for it, else use attachment instance
         if(mData.type == visualModelComponentOrAnimAttach.AttachType.ModelRef)
         {
            foreach (visualModel curModel in mVisualEditorPage.visualFile.model)
            {
               if (string.Compare(curModel.name, mData.name, true) == 0)
               {
                  if((curModel.component != null) && (curModel.component.asset != null))
                  {
                     instance = curModel.component.asset.mInstance;
                  }
               }
            }
         }
         else
         {
            instance = mData.mInstance;
         }

         if (instance != null)
         {
            List<string> boneList = new List<string>();
            boneList.Clear();
            instance.getBoneNames(ref boneList);

            // check all tags
            foreach (string curBoneName in boneList)
            {
               if (!String.IsNullOrEmpty(curBoneName))
                  fromBoneComboBox.Items.Add(curBoneName);
            }
         }
 
         
         modelRefComboBox.Items.Clear();
         foreach (visualModel model in mVisualEditorPage.visualFile.model)
         {
            if (!String.IsNullOrEmpty(model.name))
            {
               // skip model we are currently adding the attachment to (you can't attach to yourself)
               if((model.component == null) || (!model.component.attach.Contains(mData)))
               {
                  modelRefComboBox.Items.Add(model.name);
               }
            }
         }
      }


      // ---------------------------------
      // Even Handlers
      // ---------------------------------
      private void modelRefComboBox_SelectedIndexChanged(object sender, EventArgs e)
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

      private void fromBoneComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void syncAnimsCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void disregardBoneOrientationCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }
   }
}
