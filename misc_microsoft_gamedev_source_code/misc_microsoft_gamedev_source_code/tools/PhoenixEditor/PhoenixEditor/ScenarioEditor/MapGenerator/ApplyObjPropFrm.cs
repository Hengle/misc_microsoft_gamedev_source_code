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
   public partial class ApplyObjPropFrm : UserControl
   {
      ApplyObjectTreeNode mOwnerNode = null;


      public ApplyObjPropFrm()
      {
         InitializeComponent();

         numberToPlaceSlider.Setup(1, 1000, false);
         numberToPlaceSlider.NumericValue = 100;
         numberToPlaceSlider.ValueChanged += new EventHandler(numberToPlaceSlider_ValueChanged);
      }

      
      public void setOwner(ApplyObjectTreeNode owner)
      {
         mOwnerNode = owner;
         if (mOwnerNode.mData.Object == null)
            objectComboList.SelectedIndex = 0;
         else
            objectComboList.SelectedIndex = objectComboList.Items.IndexOf(mOwnerNode.mData.Object as string);
         numberToPlaceSlider.NumericValue = mOwnerNode.mData.NumberToPlace;
      }
 
      private void ApplyObjPropFrm_Load(object sender, EventArgs e)
      {
         objectComboList.Items.Clear();
         objectComboList.Items.AddRange(TriggerSystemMain.mSimResources.mProtoObjectData.mProtoObjectList.ToArray());
      }

      private void objectComboList_SelectedIndexChanged(object sender, EventArgs e)
      {
         mOwnerNode.mData.Object = (string)objectComboList.SelectedItem;
      }
      void numberToPlaceSlider_ValueChanged(object sender, EventArgs e)
      {
         mOwnerNode.mData.NumberToPlace = (int)numberToPlaceSlider.NumericValue;
      }

   }


}
