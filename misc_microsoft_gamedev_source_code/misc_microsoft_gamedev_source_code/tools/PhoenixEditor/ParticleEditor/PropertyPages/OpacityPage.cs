using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace ParticleSystem
{
   public partial class OpacityPage : UserControl
   {

      public OpacityPage()
      {
         InitializeComponent();
      }

      private ParticleEmitter data;
      private bool bInitialized = false;

      public void setData(ParticleEmitter e)
      {
         bInitialized = false;
         data = e;

         scalarProgressionControl2.AxisMinY = 0;
         scalarProgressionControl2.AxisMaxY = 100;
         scalarProgressionControl2.ChartStartColor = Color.Blue;
         scalarProgressionControl2.ChartEndColor = Color.Cyan;
         scalarProgressionControl2.LoopControl = true;
         scalarProgressionControl2.ProgressionName = "Opacity";

         //-- setup default ramp for opacity
         if (data.Opacity.Progression.Stages.Count == 0)
         {
            data.Opacity.Progression.addStage(0, 0, 0);
            data.Opacity.Progression.addStage(100, 0, 50);
            data.Opacity.Progression.addStage(0, 0, 100);
         }

         scalarProgressionControl2.setData(data.Opacity.Progression);
         getModifiedData();
         updateEnableStates();
         bInitialized = true;
      }

      private void getModifiedData()
      {
         winCheckBox1.Checked = data.Opacity.UseProgression;
         numericUpDown1.Value = (decimal)Math.Round((decimal)data.Opacity.Value, 2, MidpointRounding.ToEven);
         numericUpDown2.Value = (decimal)Math.Round((decimal)data.Opacity.ValueVariance, 2, MidpointRounding.ToEven);
      }

      private void updateEnableStates()
      {         
         scalarProgressionControl2.Enabled = winCheckBox1.Checked;
      }

      private void numericUpDown1_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         data.Opacity.Value = (double) numericUpDown1.Value;
      }

      private void numericUpDown2_ValueChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         data.Opacity.ValueVariance = (double)numericUpDown2.Value;
      }

      private void winCheckBox1_CheckedChanged(object sender, EventArgs e)
      {
         if (!bInitialized)
            return;

         data.Opacity.UseProgression = winCheckBox1.Checked;
         updateEnableStates();
      }

      private void numericUpDown_Enter(object sender, EventArgs e)
      {
         NumericUpDown control = (NumericUpDown)sender;
         control.Select(0, control.Text.Length);
      }   
   }
}
