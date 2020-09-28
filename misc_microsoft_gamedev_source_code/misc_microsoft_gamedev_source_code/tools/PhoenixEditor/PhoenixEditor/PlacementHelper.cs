using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using SimEditor;
using EditorCore;
using PhoenixEditor.ScenarioEditor;

namespace PhoenixEditor
{
   public partial class PlacementHelper : UserControl
   {
      public PlacementHelper()
      {
         InitializeComponent();

         AlignmentSlider.NumericValue = 50;
         AlignmentSlider.Setup(0, 100, false);
      }

      private void JoinButton_Click(object sender, EventArgs e)
      {
         if (SimGlobals.getSimMain().mSelectedEditorObjects.Count < 2)
            return;

         Matrix m = SimGlobals.getSimMain().mSelectedEditorObjects[0].getMatrix();

         foreach (EditorObject obj in SimGlobals.getSimMain().mSelectedEditorObjects)
         {
            obj.setMatrix(m);
         }
         CoreGlobals.getEditorMain().mOneFrame = true;

      }

      private void SnapToVisButton_Click(object sender, EventArgs e)
      {
         float amount = ((float)AlignmentSlider.NumericValue / 100f);
         bool setRotation = SetRotationCheckBox.Checked;
         SimGlobals.getSimMain().updateHeightsFromTerrain(SimGlobals.getSimMain().mSelectedEditorObjects, setRotation, amount);
         CoreGlobals.getEditorMain().mOneFrame = true;
      }

      private void MatchRotationButton_Click(object sender, EventArgs e)
      {
         if (SimGlobals.getSimMain().mSelectedEditorObjects.Count < 2)
            return;

         Matrix m = SimGlobals.getSimMain().mSelectedEditorObjects[0].getRotationOnly();

         foreach (EditorObject obj in SimGlobals.getSimMain().mSelectedEditorObjects)
         {
            Matrix trans = m * Matrix.Translation(obj.getPosition());
            obj.setMatrix(trans);
         }
         CoreGlobals.getEditorMain().mOneFrame = true;

      }

      private void SetToOriginButton_Click(object sender, EventArgs e)
      {    
         Matrix m = Matrix.Identity;

         foreach (EditorObject obj in SimGlobals.getSimMain().mSelectedEditorObjects)
         {            
            obj.setMatrix(m);
         }
         CoreGlobals.getEditorMain().mOneFrame = true;

      }

      private void SetRotationCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         AlignmentSlider.Enabled = SetRotationCheckBox.Checked;
      }
   }
}
