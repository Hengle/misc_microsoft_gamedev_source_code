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
   public partial class AnimTagBuildingDecalPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualModelAnimAssetTag mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;

      public AnimTagBuildingDecalPage()
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

         floatSliderEdit1.Value = (float)mData.position;
         fileBrowseControl1.FileName = mData.name;

         if (!String.IsNullOrEmpty(mData.userData))
         {
            float sizeX;
            float sizeZ;

            ConvertUserDataStringToValues(mData.userData, out sizeX, out sizeZ);

            sizeXFloatSliderEdit.Value = sizeX;
            sizeZFloatSliderEdit.Value = sizeZ;
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
         afterChanges.position = (decimal)floatSliderEdit1.Value;
         afterChanges.tobone = (string)toBoneComboBox.SelectedItem;
         afterChanges.name = fileBrowseControl1.FileName;

         float sizeX = sizeXFloatSliderEdit.Value;
         float sizeZ = sizeZFloatSliderEdit.Value;

         string userData;
         ConvertUserDataValuesToString(out userData, sizeX, sizeZ);
         afterChanges.userData = userData;

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


      private void ConvertUserDataStringToValues(string userData, out float sizeX, out float sizeZ)
      {
         // Example:  userData="type=Rect value=On sizeX=53.2 sizeZ=53.2"
         //

         sizeX = 1.0f;
         sizeZ = 1.0f;
         if (userData.Length == 0)
            return;

         // TODO:  Use reg expression here
         //Regex userDataFormat = new Regex(@"(?<id>[0-9]+)\.(?<prop>.*)");  
         char[] seperator = new char[2] { ' ', '=' };
         string[] strTok = userData.Split(seperator);
         for (int i = 0; i < strTok.Length; i++)
         {
            if (strTok[i] == "sizeX")
               sizeX = Convert.ToSingle(strTok[++i]);
            else if (strTok[i] == "sizeZ")
               sizeZ = Convert.ToSingle(strTok[++i]);
         }

      }

      private void ConvertUserDataValuesToString(out string userData, float sizeX, float sizeZ)
      {
         // Example:  userData="type=Rect value=On sizeX=53.2 sizeZ=53.2"
         //

         userData = String.Format("sizeX={0} sizeZ={1}", sizeX, sizeZ);
         

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

     

      private void toBoneComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void floatSliderEdit1_ValueChanged(object sender, VisualEditor.Controls.FloatSliderEdit.ValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }


      private void fileBrowseControl1_ValueChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }
   }
}
