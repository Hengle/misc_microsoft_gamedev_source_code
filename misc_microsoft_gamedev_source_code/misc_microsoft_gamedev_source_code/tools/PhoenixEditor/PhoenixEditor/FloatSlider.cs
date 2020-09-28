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
   public partial class FloatSlider : UserControl
   {
      public FloatSlider()
      {
         InitializeComponent();

         Constraint = new ConstrainedValue(0f,1f,0.5f);
      }

      ConstrainedValue mConstraint; 
      public ConstrainedValue Constraint
      {
         set
         {
            mConstraint = value;
            Setup();
         }
         get
         {
            return mConstraint;
         }         
      }
      string mValueName = "";
      public string ValueName
      {
         set
         {
            
            mValueName = value;
            updateText();
         }

      }

      public void Setup()
      {
         if(mConstraint == null) 
            return;
         this.Minlabel.Text = mConstraint.mMin.ToString();
         this.Maxlabel.Text = mConstraint.mMax.ToString();
         setSliderValue();

      }
      void setSliderValue()
      {
         float range = (mConstraint.mMax - mConstraint.mMin);
         float fval = ((mConstraint.Value - mConstraint.mMin) / range) * 1000;
         trackBar1.Value =  (int)fval;


  

      }
      void getSliderValue()
      {
         int ival = trackBar1.Value;
         float range = (mConstraint.mMax - mConstraint.mMin);
         //float fval = ((mConstraint.Value - mConstraint.mMin) / range) * 1000;
         mConstraint.Value = (ival / 1000f) * range + mConstraint.mMin; 
      }
      public bool mbIntStep = false;
      private void trackBar1_ValueChanged(object sender, EventArgs e)
      {
         getSliderValue();
         updateText();
      }

      void updateText()
      {

          //this.Namelabel.Text = mValueName + ": " + mConstraint.Value.ToString();
         float displayVal = mConstraint.Value;
         if (mbIntStep)
            displayVal = (int)displayVal;
         this.Namelabel.Text = String.Format("{0} : {1:F2}", mValueName, displayVal);
     }

   }
}
