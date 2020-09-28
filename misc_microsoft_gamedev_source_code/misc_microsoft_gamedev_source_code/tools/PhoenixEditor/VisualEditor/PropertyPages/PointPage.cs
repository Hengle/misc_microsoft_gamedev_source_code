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
   public partial class PointPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualModelComponentPoint mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;

      public PointPage()
      {
         InitializeComponent();

         // Initialize combo boxes
         Type enumtype = typeof(visualModelComponentPoint.ComponentPointType);
         foreach (string s in Enum.GetNames(enumtype))
            typeComboBox.Items.Add(s);

         typeComboBox.SelectedIndex = 0;

         enumtype = typeof(visualModelComponentPoint.ComponentPointData);
         foreach (string s in Enum.GetNames(enumtype))
            dataComboBox.Items.Add(s);

         dataComboBox.SelectedIndex = 0;
      }

      public void setVisualEditorPage(VisualEditorPage page)
      {
         mVisualEditorPage = page;
      }

      public void bindData(visualModelComponentPoint point, TreeNode treeNode)
      {
         mIsBindingData = true;

         mData = point;
         mNode = treeNode;


         // Move data to control data (DATA -> CONTROL DATA)
         //
         refreshComboBoxItems();

         typeComboBox.SelectedIndex = (int)mData.pointType;
         dataComboBox.SelectedIndex = (int)mData.pointData;

         bool found = false;
         for (int i = 0; i < toBoneComboBox.Items.Count; i++)
         {
            if (String.Compare(mData.bone, (string)toBoneComboBox.Items[i], true) == 0)
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
         visualModelComponentPoint afterChanges = new visualModelComponentPoint();
         afterChanges.pointType = (visualModelComponentPoint.ComponentPointType)typeComboBox.SelectedIndex;
         afterChanges.pointData = (visualModelComponentPoint.ComponentPointData)dataComboBox.SelectedIndex;
         afterChanges.bone = (string)toBoneComboBox.SelectedItem;

         // Add/Execute undo action
         UndoRedoChangeDataAction undoAction = new UndoRedoChangeDataAction(mData, afterChanges);
         mVisualEditorPage.mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
      }



      public void refreshComboBoxItems()
      {
         toBoneComboBox.Items.Clear();

         // Get model we are attaching to
         TreeNode componentNode = mNode.Parent;
         TreeNode modelNode = componentNode.Parent;

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
      private void typeComboBox_SelectedIndexChanged(object sender, EventArgs e)
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

      private void dataComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }
   }
}
