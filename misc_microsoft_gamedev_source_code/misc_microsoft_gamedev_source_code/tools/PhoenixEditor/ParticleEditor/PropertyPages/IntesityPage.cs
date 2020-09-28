using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace ParticleSystem
{
   public partial class IntensityPage : UserControl
   {
      public IntensityPage()
      {
         InitializeComponent();
      }
      private ParticleEmitter data;
      private bool bInitialized = false;

      public void setData(ParticleEmitter e)
      {
         bInitialized = false;
         data = e;

         scalarProgressionControl1.AxisMinY = 0;
         scalarProgressionControl1.AxisMaxY = 100;
         scalarProgressionControl1.ChartStartColor = Color.Purple;
         scalarProgressionControl1.ChartEndColor = Color.LightPink;
         scalarProgressionControl1.ProgressionName = "Intensity";
         scalarProgressionControl1.LoopControl = true;
         scalarProgressionControl1.setData(data.Intensity.Progression);
         getModifiedData();
         updateEnableStates();
         bInitialized = true;
      }

      private void getModifiedData()
      {
         checkBox1.Checked = data.Intensity.UseProgression;
         numericUpDown1.Value = (decimal)Math.Round((decimal)data.Intensity.Value, 2, MidpointRounding.ToEven);
         numericUpDown2.Value = (decimal)Math.Round((decimal)data.Intensity.ValueVariance, 2, MidpointRounding.ToEven);
      }

      private void updateEnableStates()
      {
         scalarProgressionControl1.Enabled = checkBox1.Checked;
      }

      private void numericUpDown1_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         data.Intensity.Value = (double) numericUpDown1.Value;
      }

      private void numericUpDown2_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         data.Intensity.ValueVariance = (double)numericUpDown2.Value;
      }

      private void winCheckBox1_CheckedChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         data.Intensity.UseProgression = checkBox1.Checked;
         updateEnableStates();
      }

      private void numericUpDown_Enter(object sender, EventArgs e)
      {
         NumericUpDown control = (NumericUpDown)sender;
         control.Select(0, control.Text.Length);
      }   
   }
}
