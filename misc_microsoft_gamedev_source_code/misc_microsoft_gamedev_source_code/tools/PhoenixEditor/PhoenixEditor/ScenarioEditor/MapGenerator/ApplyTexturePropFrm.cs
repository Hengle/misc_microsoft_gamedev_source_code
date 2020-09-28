using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using EditorCore;
using SimEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ApplyTexturePropFrm : UserControl
   {
      ApplyTextureTreeNode mOwnerNode = null;
      public ApplyTexturePropFrm()
      {
         InitializeComponent();

         intensitySlider.Setup(0, 1, true);
         intensitySlider.NumericValue = 1;
         intensitySlider.ValueChanged += new EventHandler(intensitySlider_ValueChanged);
      }

      
      private void ApplyTexturePropFrm_Load(object sender, EventArgs e)
      {
         textureComboList.Items.Clear();
         textureComboList.Items.AddRange(SimTerrainType.getTerrainTextures().ToArray());
      }

      public void setOwner(ApplyTextureTreeNode owner)
      {
         mOwnerNode = owner;
         if (mOwnerNode.mData.Texture == null)
            textureComboList.SelectedIndex = 0;
         else
            textureComboList.SelectedIndex = textureComboList.Items.IndexOf(mOwnerNode.mData.Texture as string);
      }

      void intensitySlider_ValueChanged(object sender, EventArgs e)
      {
         //mOwnerNode.inte = (int)numberToPlaceSlider.NumericValue;
      }

      private void textureComboList_SelectedIndexChanged(object sender, EventArgs e)
      {
         mOwnerNode.mData.Texture = (string)textureComboList.SelectedItem;
      }

   }
}
