using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace ParticleSystem
{
   public partial class SpeedPage : UserControl
   {
      public SpeedPage()
      {
         InitializeComponent();
      }

      private ParticleEmitter data;
      private bool bInitialized = false;

      public void setData(ParticleEmitter e)
      {
         bInitialized = false;
         data = e;
         float minY = -100;
         float maxY = 100;

         float defaultValue0 = 100;
         float defaultValue1 = 100;
         bool enableLooping = false;
         vectorProgressionControl1.setData(e.Speed.Progression, minY, maxY, defaultValue0, defaultValue1, enableLooping);
         getModifiedData();
         bInitialized = true;
         updateEnableState();
         
      }

      private void updateEnableState()
      {
         vectorProgressionControl1.EnableXProgression(data.Speed.UseXProgression);
         vectorProgressionControl1.EnableYProgression(data.Speed.UseYProgression);
         vectorProgressionControl1.EnableZProgression(data.Speed.UseZProgression);
      }

      private void setModifiedData()
      {
         if (!bInitialized)
            return;

         data.Speed.ValueX = (double)numericUpDown1.Value;
         data.Speed.ValueY = (double)numericUpDown2.Value;
         data.Speed.ValueZ = (double)numericUpDown3.Value;

         data.Speed.ValueXVariance = (double)numericUpDown4.Value;
         data.Speed.ValueYVariance = (double)numericUpDown5.Value;
         data.Speed.ValueZVariance = (double)numericUpDown6.Value;

         data.Speed.UseXProgression = checkBox1.Checked;
         data.Speed.UseYProgression = checkBox2.Checked;
         data.Speed.UseZProgression = checkBox3.Checked;
      }

      private void getModifiedData()
      {
         numericUpDown1.Value= (decimal)Math.Round((decimal)data.Speed.ValueX, 2, MidpointRounding.ToEven);
         numericUpDown2.Value= (decimal)Math.Round((decimal)data.Speed.ValueY, 2, MidpointRounding.ToEven);
         numericUpDown3.Value= (decimal)Math.Round((decimal)data.Speed.ValueZ, 2, MidpointRounding.ToEven);

         numericUpDown4.Value = (decimal)Math.Round((decimal)data.Speed.ValueXVariance, 2, MidpointRounding.ToEven);
         numericUpDown5.Value = (decimal)Math.Round((decimal)data.Speed.ValueYVariance, 2, MidpointRounding.ToEven);
         numericUpDown6.Value = (decimal)Math.Round((decimal)data.Speed.ValueZVariance, 2, MidpointRounding.ToEven);

         checkBox1.Checked = data.Speed.UseXProgression;
         checkBox2.Checked = data.Speed.UseYProgression;
         checkBox3.Checked = data.Speed.UseZProgression;
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
