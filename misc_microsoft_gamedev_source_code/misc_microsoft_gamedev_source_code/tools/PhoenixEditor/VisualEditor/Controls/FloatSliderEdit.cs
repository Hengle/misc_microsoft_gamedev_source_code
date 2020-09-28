using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Globalization;


namespace VisualEditor.Controls
{
   public partial class FloatSliderEdit : UserControl
   {
      // Boolean flag used to determine when a character other than a number is entered.
      private bool  mNonNumberEnteredInTextBox = false;
      private float mValue = 0.0f;

      private float mMinValue = 0.0f;
      private float mMaxValue = 1.0f;

      private int mNumDecimals = 3;


      public float Value
      {
         get
         {
            return mValue;
         }

         set
         {
            mValue = value;

            // Clamp to the right extends
            if (mValue > mMaxValue)
               mValue = mMaxValue;
            if (mValue < mMinValue)
               mValue = mMinValue;

            updateTrackBar();
            updateTextBox();
         }
      }

      public float MaxValue
      {
         get { return mMaxValue; }
         set 
         { 
            mMaxValue = value;
            if (mMaxValue < mMinValue)
               mMaxValue = mMinValue;
         }
      }

      public float MinValue
      {
         get { return mMinValue; }
         set 
         { 
            mMinValue = value;
            if (mMinValue > mMaxValue)
               mMinValue = mMaxValue;
         }
      }

      public int NumDecimals
      {
         get { return mNumDecimals; }
         set { mNumDecimals = value; }
      }

      public delegate void ValueChangedDelegate(object sender, ValueChangedEventArgs e);
      public event ValueChangedDelegate ValueChanged;

      public class ValueChangedEventArgs : EventArgs
      {
         private float mValue;
         public float Value
         {
            get { return mValue; }
            set { mValue = value; }
         }

         public ValueChangedEventArgs(float valueIn)
         {
            Value = valueIn;
         }
      }



      public FloatSliderEdit()
      {
         InitializeComponent();
      }

      private void updateTrackBar()
      {
         trackBar1.Value = (int)(((mValue - mMinValue) / (mMaxValue - mMinValue)) * 1000.0f);
      }

      private void updateTextBox()
      {
         string stringValue;
         switch(mNumDecimals)
         {
            case 0: stringValue = mValue.ToString("0;-0;0"); break;
            case 1: stringValue = mValue.ToString("0.0;-0.0;0.0"); break;
            case 2: stringValue = mValue.ToString("0.00;-0.00;0.00"); break;
            default: stringValue = mValue.ToString("0.000;-0.000;0.000"); break;
         }
         textBox1.Text = stringValue;
      }

      #region TrackBar Handlers

      private void trackBar1_Scroll(object sender, EventArgs e)
      {
         float trackValue = (trackBar1.Value / 1000.0f);
         trackValue = (trackValue * (mMaxValue - mMinValue)) + mMinValue;

         trackValue = (float)Math.Round((decimal)trackValue, mNumDecimals);

         mValue = trackValue;
         
         // update text
         updateTextBox();

         // call event
         ValueChangedEventArgs args = new ValueChangedEventArgs(mValue);
         if (ValueChanged != null)
            ValueChanged(this, args);
      }
      #endregion

      #region TextBox Handlers

      private void textBox1_KeyDown(object sender, KeyEventArgs e)
      {
         // Initialize the flag to false.
         mNonNumberEnteredInTextBox = false;

         // Determine whether the keystroke is a number from the top of the keyboard.
         if (e.KeyCode < Keys.D0 || e.KeyCode > Keys.D9)
         {
            // Determine whether the keystroke is a number from the keypad.
            if (e.KeyCode < Keys.NumPad0 || e.KeyCode > Keys.NumPad9)
            {
               // Determine whether the keystroke is a backspace.
               if (e.KeyCode != Keys.Back)
               {
                  // Determine whether the keystroke is a period.
                  if (e.KeyCode != Keys.OemPeriod)
                  {
                     // A non-numerical keystroke was pressed.
                     // Set the flag to true and evaluate in KeyPress event.
                     mNonNumberEnteredInTextBox = true;
                  }
               }
            }
         }
      }
      
      private void textBox1_KeyUp(object sender, KeyEventArgs e)
      {
         // Check for the flag being set in the KeyDown event.
         if (mNonNumberEnteredInTextBox == true)
         {
            // Stop the character from being entered into the control since it is non-numerical.
            e.Handled = true;
         }


         double value = 0.0f;
         bool isValid = Double.TryParse(textBox1.Text, out value);

         if (isValid)
         {
            mValue = (float)value;

            // Clamp to the right extends
            if (mValue > mMaxValue)
               mValue = mMaxValue;
            if (mValue < mMinValue)
               mValue = mMinValue;

            // update track bar
            updateTrackBar();

            // call event
            ValueChangedEventArgs args = new ValueChangedEventArgs(mValue);
            if (ValueChanged != null)
               ValueChanged(this, args);
         }
      }

      #endregion


   }
}
