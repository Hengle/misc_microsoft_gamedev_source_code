using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using Terrain;
namespace PhoenixEditor.Filter_Dialogs
{
   public partial class maskfromSlope : Form
   {
      public maskfromSlope()
      {
         InitializeComponent();
         this.TopMost = true;
      }

      private void trackBar1_Scroll(object sender, EventArgs e)
      {
         
         label1.Text = "Angle to select:" + trackBar1.Value + "degrees";
      }

      private void button1_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      private void button2_Click(object sender, EventArgs e)
      {
         Masking.clearSelectionMask();
         float angleVal = (float)(trackBar1.Maximum - trackBar1.Value) / (float)trackBar1.Maximum;
         float range = 0.1f + (float)( (trackBar2.Value)  ) / (float)trackBar1.Maximum;
         Masking.createSelectionMaskFromTerrain(-100,100,angleVal, range);
         this.Close();
      }
   }
}