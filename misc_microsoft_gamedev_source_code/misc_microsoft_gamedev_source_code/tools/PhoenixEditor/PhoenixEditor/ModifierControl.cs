using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using Terrain;

namespace PhoenixEditor
{
   public partial class ModifierControl : UserControl
   {
      public ModifierControl()
      {
         InitializeComponent();
      }

      Modifier mModifierField;
      public Modifier mModifier
      {
         set
         {
            mModifierField = value;
            this.Namelabel.Text = mModifierField.mName;
            //this.InputComboBox

            this.floatSliderMin.Constraint = mModifierField.mMin;
            this.floatSliderMin.ValueName = "Min Multiplier";
            this.floatSliderMax.Constraint = mModifierField.mMax;
            this.floatSliderMax.ValueName = "Max Multiplier";

            checkBox1.Checked = (mModifierField.mInputType == 1)? true: false;
         }
         get
         {
            return mModifierField;
         }
      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         if(checkBox1.Checked == true)
         {
            mModifierField.mInputType = Modifier.cPressure;
         }
         else
         {
            mModifierField.mInputType = Modifier.cNone;
         }
      }
   }
}
