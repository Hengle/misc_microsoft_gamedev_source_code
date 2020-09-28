using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace ParticleSystem
{
   public partial class ScalePage : UserControl
   {
      public ScalePage()
      {
         InitializeComponent();
      }

      private ParticleEmitter data;
      private bool bInitialized = false;

      public void setData(ParticleEmitter e)
      {
         bInitialized = false;
         data = e;
         float minY = 0;
         float maxY = 100;
         float defaultValue0 = 0;
         float defaultValue1 = 100;
         bool enableLooping = true;
         vectorProgressionControl1.setData(e.Scale.Progression, minY, maxY, defaultValue0, defaultValue1, enableLooping);
         getModifiedData();
         bInitialized = true;
         updateEnableState();         
      }

      private void updateEnableState()
      {
         vectorProgressionControl1.EnableXProgression(data.Scale.UseXProgression);
         vectorProgressionControl1.EnableYProgression(data.Scale.UseYProgression);
         vectorProgressionControl1.EnableZProgression(data.Scale.UseZProgression);
      }

      private void setModifiedData()
      {
         if (!bInitialized)
            return;

         data.Scale.UniformValue = (double)numericUpDown7.Value;
         data.Scale.ValueX = (double)numericUpDown1.Value;
         data.Scale.ValueY = (double)numericUpDown2.Value;
         data.Scale.ValueZ = (double)numericUpDown3.Value;

         data.Scale.UniformValueVariance = (double)numericUpDown8.Value;
         data.Scale.ValueXVariance = (double)numericUpDown4.Value;
         data.Scale.ValueYVariance = (double)numericUpDown5.Value;
         data.Scale.ValueZVariance = (double)numericUpDown6.Value;

         data.Scale.UseXProgression = checkBox1.Checked;
         data.Scale.UseYProgression = checkBox2.Checked;
         data.Scale.UseZProgression = checkBox3.Checked;
      }

      private void getModifiedData()
      {       
         numericUpDown1.Value = (decimal)Math.Round((decimal)data.Scale.ValueX, 2, MidpointRounding.ToEven);
         numericUpDown2.Value = (decimal)Math.Round((decimal)data.Scale.ValueY, 2, MidpointRounding.ToEven);
         numericUpDown3.Value = (decimal)Math.Round((decimal)data.Scale.ValueZ, 2, MidpointRounding.ToEven);
         numericUpDown7.Value = (decimal)Math.Round((decimal)data.Scale.UniformValue, 2, MidpointRounding.ToEven);

         numericUpDown4.Value = (decimal)Math.Round((decimal)data.Scale.ValueXVariance, 2, MidpointRounding.ToEven);
         numericUpDown5.Value = (decimal)Math.Round((decimal)data.Scale.ValueYVariance, 2, MidpointRounding.ToEven);
         numericUpDown6.Value = (decimal)Math.Round((decimal)data.Scale.ValueZVariance, 2, MidpointRounding.ToEven);
         numericUpDown8.Value = (decimal)Math.Round((decimal)data.Scale.UniformValueVariance, 2, MidpointRounding.ToEven);

         checkBox1.Checked = data.Scale.UseXProgression;
         checkBox2.Checked = data.Scale.UseYProgression;
         checkBox3.Checked = data.Scale.UseZProgression;
      }

      private void checkBox_CheckedChanged(object sender, EventArgs e)
      {
         setModifiedData();
         updateEnableState();
      }

      private void numericUpDown1_ValueChanged(object sender, EventArgs e)
      {
         setModifiedData();
      }

      private void numericUpDown_Enter(object sender, EventArgs e)
      {
         NumericUpDown control = (NumericUpDown)sender;
         control.Select(0, control.Text.Length);
      }   
   }
}
