using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace ParticleSystem
{
   public partial class UVControl : UserControl
   {
      bool bInitialized = false;
      private UVAnimation data;
      public UVControl()
      {
         InitializeComponent();
      }

      public void setData(UVAnimation e)
      {
         bInitialized = false;
         data = e;
         getStoredData();
         bInitialized = true;
      }

      private void getStoredData()
      {
         numericUpDown1.Value = data.NumFrames;
         numericUpDown2.Value = data.FramesPerSecond;
         numericUpDown3.Value = data.FrameWidth;
         numericUpDown4.Value = data.FrameHeight;
         numericUpDown5.Value = (decimal)Math.Round((decimal)data.ScrollU, 2, MidpointRounding.ToEven);
         numericUpDown6.Value = (decimal)Math.Round((decimal)data.ScrollV, 2, MidpointRounding.ToEven);
         winCheckBox1.Checked = data.UVAnimationEnabled;
         checkBox1.Checked = data.UseRandomScrollOffsetU;
         checkBox2.Checked = data.UseRandomScrollOffsetV;

         updateEnableState();
      }

      private void setStoredData()
      {
         if (!bInitialized)
            return;

         data.NumFrames = (int) numericUpDown1.Value;
         data.FramesPerSecond = (int) numericUpDown2.Value;
         data.FrameWidth = (int) numericUpDown3.Value;
         data.FrameHeight = (int) numericUpDown4.Value;
         data.UVAnimationEnabled = winCheckBox1.Checked;
         data.ScrollU = (double) numericUpDown5.Value;
         data.ScrollV = (double) numericUpDown6.Value;
         data.UseRandomScrollOffsetU = checkBox1.Checked;
         data.UseRandomScrollOffsetV = checkBox2.Checked;

         updateEnableState();
      }

      private void updateEnableState()
      {
         numericUpDown1.Enabled = winCheckBox1.Checked;
         numericUpDown2.Enabled = winCheckBox1.Checked;
         numericUpDown3.Enabled = winCheckBox1.Checked;
         numericUpDown4.Enabled = winCheckBox1.Checked;
      }

      private void winCheckBox1_CheckedChanged(object sender, EventArgs e)
      {
         updateEnableState();
         data.UVAnimationEnabled = winCheckBox1.Checked;
      }

      private void numericUpDown_ValueChanged(object sender, EventArgs e)
      {
         setStoredData();
      }

      private void numericUpDown_Enter(object sender, EventArgs e)
      {
         NumericUpDown control = (NumericUpDown)sender;
         control.Select(0, control.Text.Length);
      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {         
         data.UseRandomScrollOffsetU = checkBox1.Checked;
      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         data.UseRandomScrollOffsetV = checkBox2.Checked;
      }      
   }
}
