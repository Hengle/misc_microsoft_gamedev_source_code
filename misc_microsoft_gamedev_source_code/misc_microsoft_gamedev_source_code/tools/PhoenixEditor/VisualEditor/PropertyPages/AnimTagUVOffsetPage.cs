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
   public partial class AnimTagUVOffsetPage : UserControl
   {
      private bool mIsBindingData = false;
      private VisualEditorPage mVisualEditorPage = null;
      private visualModelAnimAssetTag mData = null;
      private TreeNode mNode = null;
      public AnimTagUVOffsetPage()
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
         
         floatSliderEdit1.Value = (float)mData.position;

         if (!String.IsNullOrEmpty(mData.userData))
         {
            float sizeX;
            float sizeZ;

            ConvertUserDataStringToValues(mData.userData, out sizeX, out sizeZ);

            sizeXFloatSliderEdit.Value = sizeX;
            sizeZFloatSliderEdit.Value = sizeZ;
            
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

      private void ConvertUserDataStringToValues(string userData, out float sizeX, out float sizeZ)
      {
         // Example:  userData="u0Offset=53.2 v0Offset=53.2"
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
            if (strTok[i] == "u0Offset")
               sizeX = Convert.ToSingle(strTok[++i]);
            else if (strTok[i] == "v0Offset")
               sizeZ = Convert.ToSingle(strTok[++i]);
         }

      }

      private void ConvertUserDataValuesToString(out string userData, float sizeX, float sizeZ)
      {
         // Example:  userData="uOffset=53.2 vOffset=53.2"
         //

         userData = String.Format("u0Offset={0} v0Offset={1}", sizeX, sizeZ);
        
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
   }
}
