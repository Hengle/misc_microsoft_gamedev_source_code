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
   public partial class terraceTerrain : Form
   {
      public terraceTerrain()
      {
         InitializeComponent();
         this.TopMost = true;
      }

      private void button2_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      private void button1_Click(object sender, EventArgs e)
      {
         TerraceFilter te = new TerraceFilter();
         te.mNumTerraces = (int)(numericUpDown1.Value);
         te.apply(checkBox1.Checked);
         te = null;

         this.Close();
      }
   }
}