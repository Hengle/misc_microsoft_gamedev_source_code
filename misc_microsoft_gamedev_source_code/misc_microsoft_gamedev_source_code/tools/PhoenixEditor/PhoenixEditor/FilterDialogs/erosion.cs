using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using Terrain;
using EditorCore;
using NoiseGeneration;


namespace PhoenixEditor.Filter_Dialogs
{
   public partial class erosion : Form
   {
      public erosion()
      {
         InitializeComponent();
         this.TopMost = true;
      }

      private void trackBar1_Scroll(object sender, EventArgs e)
      {
      }

      private void button1_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      private void button2_Click(object sender, EventArgs e)
      {
         if(tabControl1.SelectedIndex==0) 
         {
            //THERMAL EROSION!!!
            ThermalErosionFilter te = new ThermalErosionFilter();
            te.mNumIterations = (int)numericUpDown1.Value;
            te.mTalusAngle = (float)(trackBar1.Maximum - trackBar1.Value) / 100.0f;
            te.mType = radioButton1.Checked ? 0 : 1;
            te.apply(checkBox1.Checked);
            te = null;
 
         }
         else if(tabControl1.SelectedIndex==1)
         {
            //HYDRAULIC EROSION
            BasicHydraulicErosionFilter ef = new BasicHydraulicErosionFilter();
            ef.mSedimentCapacity = trackBar7.Value;
            ef.mSoilSoftness = 1.0f / trackBar2.Value;
            ef.mRainAmt = 1.0f / trackBar3.Value;
            ef.mRainDropSize = 1.0f / trackBar4.Value;
            ef.mDepositionAmt = 1.0f / trackBar5.Value;
            ef.mEvaporationRate = 1.0f / trackBar6.Value;
            ef.mNumIterations = (int)numericUpDown2.Value;

            ef.apply(checkBox1.Checked);
            ef = null;

         }

         this.Close();
      }

      private void trackBar2_Scroll(object sender, EventArgs e)
      {
         label5.Text = "Terrain Softness (" + (trackBar2.Value / 100.0f) + ")";
      }
   }
}