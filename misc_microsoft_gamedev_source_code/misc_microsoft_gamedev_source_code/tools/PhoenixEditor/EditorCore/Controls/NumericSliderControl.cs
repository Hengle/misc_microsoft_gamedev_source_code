using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore
{
   public partial class NumericSliderControl : UserControl
   {
      public NumericSliderControl()
      {
         InitializeComponent();
         trackBar1.Maximum = 10000;
         trackBar1.Minimum = 0;
      }

      bool mbSkip = false;
      private bool mbStartGray = false;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public bool StartGray
      {
         get
         {
            return mbStartGray;
         }
         set
         {
            if(mbSkip == true )
            {
               mbSkip = false;
               return;
            }
            mbStartGray = value;
            if (mbStartGray == true)
            {
               mbSkip = true;
               this.textBox1.BackColor = System.Drawing.Color.LightGray;
            }
            else
            {
               this.textBox1.BackColor = System.Drawing.Color.White;
            }
         }
      }


      private bool mLogrithmic = false;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public bool Logrithmic 
      {
         get
         {
            return mLogrithmic;
         }
         set
         {
            mLogrithmic = value;
         }
      }


      public double mNumericValue = 0;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public double NumericValue
      {
         get
         {
            return mNumericValue;
         }
         set
         {
            mNumericValue = value;
            if (ValueChanged != null)
            {
               ValueChanged(this, null);
            }

            //trackBar1.Value = (int)((mNumericValue - mMin) / mMultiplier);
            if (mNumericValue == 0)
            {
               textBox1.Text = "0";
            }
            else
            {
               textBox1.Text = string.Format("{0:F3}", mNumericValue);//.TrimEnd(new char[]{'0'});

            }
         }
      }

      private void LocalValueUpdate(double value)
      {
         if (StartGray == true)
         {
            StartGray = false;

         }

         mNumericValue = value;
         if (ValueChanged != null)
         {
            ValueChanged(this, null);
         }

      }

      public event EventHandler ValueChanged;

      double mMin = 0;
      double mMax = 1;
      double mMultiplier = 1/10000.0;
      bool mbFloating = true;

      public void Setup(double min, double max, bool bFloating)
      {
         mMin = min;
         mMax = max;
         mbFloating = bFloating;

         if (Logrithmic == false)
         {
            mMultiplier = (mMax - mMin) / 10000;
         }
         else
         {
            mMultiplier = Math.Log10(1 + mMax - mMin) / ((10000));
         }

         NumericValue = mMin;
      }

      private void trackBar1_Scroll(object sender, EventArgs e)
      {
         double val = 0;
         if (Logrithmic == false)
         {
            val = (trackBar1.Value * mMultiplier) + mMin;
         }
         else
         {
            val = Math.Pow(10,((trackBar1.Value) * mMultiplier)) - 1 + mMin;
         }
         if (mbFloating == false)
         {
            val = (int)val;
            textBox1.Text = ((int)val).ToString();
         }
         else
         {
            
            textBox1.Text = string.Format("{0:F3}", val);//val.ToString("");
         }
      }

      private void textBox1_TextChanged(object sender, EventArgs e)
      {
         double res = 0;

         if (textBox1.Text.EndsWith("."))
         {
            textBox1.ForeColor = Color.Red;
            return;
         }

         if (double.TryParse(textBox1.Text, out res))
         {
            textBox1.ForeColor = Color.Black;
            //NumericValue = res;
            LocalValueUpdate(res);
         }
         else
         {
            textBox1.ForeColor = Color.Red;
            return;
         }


         int value = 0;
         
         if (Logrithmic == false)
         {
            value = (int)((mNumericValue - mMin) / mMultiplier);
         }
         else
         {
            value = (int)(Math.Log10((mNumericValue - (mMin-1))) / mMultiplier);
         }
         if(value > 10000)
         {
            value = 10000;
         }
         if(value < 0)
         {
            value = 0;
         }

         trackBar1.Value = value;
      }

      public void SetInvalid(bool invalid)
      {
         
         if(invalid)
         {
            textBox1.ForeColor = Color.Red;
         }
         else
         {
            textBox1.ForeColor = Color.Black;
         }
      }
   }
}
