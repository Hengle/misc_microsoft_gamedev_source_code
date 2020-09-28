using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore
{
   public partial class TimeEditor : UserControl
   {
      public TimeEditor()
      {
         InitializeComponent();

         

      }
      public event EventHandler ValueChanged;

      bool mbSusbendUI = false;
      private void MinutesTextBox_TextChanged(object sender, EventArgs e)
      {
         string text = ((TextBox)(sender)).Text;
         float value;

         if (text.EndsWith("."))
         {
            return;
         }
         if(float.TryParse(text,out value))
         {
            //mMilliSeconds = (1000 * value * 60.0f);
            //mSeconds = (value * 60.0f);
            //mMinutes = value;

            //UpdateUI();

            Minutes = value;
         }

      }



      private void SecondsTextBox_TextChanged(object sender, EventArgs e)
      {   
         string text = ((TextBox)(sender)).Text;
         float value;

         if (text.EndsWith("."))
         {
            return;
         }
         if (float.TryParse(text, out value))
         {
            //mMilliSeconds = (1000 * value);
            //mSeconds = value;
            //mMinutes = (value / 60.0f);

            //UpdateUI();
            Seconds = value;
         }      
      }

      private void MillisecondsTextBox_TextChanged(object sender, EventArgs e)
      {
         string text = ((TextBox)(sender)).Text;
         float value;

         if (text.EndsWith("."))
         {
            return;
         }
         if (float.TryParse(text, out value))
         {
            //mMilliSeconds = value;
            //mSeconds = (0.001f * value);
            //mMinutes = (0.001f * value * 60.0f);

            //UpdateUI();
            MilliSeconds = value;
         }

      }

      private void UpdateUI()
      {
         if (mbSusbendUI == true)
            return;
         mbSusbendUI = true;

         if (MinutesTextBox.Text != mMinutes.ToString())
            MinutesTextBox.Text = mMinutes.ToString();
         if (SecondsTextBox.Text != mSeconds.ToString())
            SecondsTextBox.Text = mSeconds.ToString();
         if (MillisecondsTextBox.Text != mMilliSeconds.ToString())
            MillisecondsTextBox.Text = mMilliSeconds.ToString();

         if (ValueChanged!=null)
         {
            ValueChanged.Invoke(this, null);
         }
         mbSusbendUI = false;
      }

      float mSeconds = 0;
      public float Seconds
      {
         get
         {
            return mSeconds;
         }
         set
         {
            mSeconds = value;

            mMilliSeconds = (1000 * value);
            mSeconds = value;
            mMinutes = (value / 60.0f);

            UpdateUI();
         }
      }
      float mMinutes = 0;
      public float Minutes
      {
         get
         {
            return mMinutes;
         }
         set
         {
            mMinutes = value;

            mMilliSeconds = (1000 * value * 60.0f);
            mSeconds = (value * 60.0f);
            mMinutes = value;

            UpdateUI();
         }
      }
      float mMilliSeconds = 0;
      public float MilliSeconds
      {
         get
         {
            return mMilliSeconds;
         }
         set
         {
            mMilliSeconds = value;


            mMilliSeconds = value;
            mSeconds = (0.001f * value);
            mMinutes = (0.001f * value / 60.0f);

            UpdateUI();
         }
      }

   }
}
