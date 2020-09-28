using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ApplyTransPropFrm : UserControl
   {
      ApplyTransformTreeNode mOwnerNode = null;

      public ApplyTransPropFrm()
      {
         InitializeComponent();

         heightOffsetSlider.Setup(-300, 300, true);
         heightOffsetSlider.NumericValue = 0;
         heightOffsetSlider.ValueChanged += new EventHandler(heightOffsetSlider_ValueChanged);
      }


      public void setOwner(ApplyTransformTreeNode owner)
      {
         mOwnerNode = owner;

         heightOffsetSlider.NumericValue = mOwnerNode.mData.YOffset;
      }


      void heightOffsetSlider_ValueChanged(object sender, EventArgs e)
      {
         mOwnerNode.mData.YOffset = (int)heightOffsetSlider.NumericValue;
      }

      private void numberToPlaceSlider_Load(object sender, EventArgs e)
      {

      }

      private void label4_Click(object sender, EventArgs e)
      {

      }

      private void ApplyTransPropFrm_Load(object sender, EventArgs e)
      {
         
      }
   }
}
