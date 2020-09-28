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
   public partial class mesaTerrain : Form
   {
      public mesaTerrain()
      {
         InitializeComponent();
         this.TopMost = true;
      }

      private void button1_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      private void button2_Click(object sender, EventArgs e)
      {
         PlateauFilter bf = new PlateauFilter();
         bf.mbutteMaxPercentThreshold.Value = (double)trackBar1.Value / 100.0f;
         bf.mbutteMinPercentThreshold.Value = (double)trackBar2.Value / 100.0f;
         bf.mHeightsDefined = checkBox2.Checked;
         bf.mMaxHeight = (float)numericUpDown1.Value;
         bf.mMinHeight = (float)numericUpDown2.Value;

         bf.apply(checkBox1.Checked);
         bf = null;


         this.Close();
      }

      private void mesaTerrain_Load(object sender, EventArgs e)
      {
         numericUpDown1.Value = 13;
         numericUpDown2.Value = 0;
      }

      private void trackBar2_Scroll(object sender, EventArgs e)
      {
         label4.Text = trackBar2.Value.ToString() + "%";
      }

      private void trackBar1_Scroll(object sender, EventArgs e)
      {
         label3.Text = trackBar1.Value.ToString() + "%";
      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         numericUpDown1.Enabled = checkBox2.Checked;
         numericUpDown2.Enabled = checkBox2.Checked;
      }
   }
}