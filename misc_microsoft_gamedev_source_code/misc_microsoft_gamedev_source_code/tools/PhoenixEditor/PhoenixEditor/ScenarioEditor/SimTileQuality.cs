using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Terrain;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class SimTileQuality : Form
   {
      public SimTileQuality()
      {
         InitializeComponent();
      }

      int visToSimMul = 0;
      private void button1_Click(object sender, EventArgs e)
      {
         TerrainCreationParams prms = new TerrainCreationParams();
         prms.initFromVisData(TerrainGlobals.getTerrain().getNumXVerts(),TerrainGlobals.getTerrain().getNumZVerts(),TerrainGlobals.getTerrain().getTileScale(),visToSimMul);

         TerrainGlobals.getEditor().getSimRep().reinit(prms.mNumSimXTiles, prms.mNumSimZTiles, prms.mSimTileSpacing, 1.0f / (float)visToSimMul);

         TerrainGlobals.getEditor().getSimRep().update(false,false);// updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), false);

         this.Close();
      }

      private void button2_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      string []SimDenText = new string[]{"Min","Low","Medium","High","Max"};
      
      private void updateLabel()
      {
         string value =SimDenText[trackBar1.Value];
         int numVisVerts = visToSimMul;
         int numSimVerts = 1;
         label1.Text = "SimDensity : " + value + "(" + numVisVerts + "vis verts per " + numSimVerts + " sim verts )";
      }

      private void SimTileQuality_Load(object sender, EventArgs e)
      {
         trackBar1.Minimum = 0;
         trackBar1.Maximum = (int)eSimQuality.cQuality_Count-1;

         int v = (int)(1.0f/TerrainGlobals.getEditor().getSimRep().getVisToSimScale());

         //ugggg...
         if (v == (int)eSimQuality.cQuality_Max)
            trackBar1.Value = 4;
         else if (v == (int)eSimQuality.cQuality_High)
            trackBar1.Value = 3;
         else if (v == (int)eSimQuality.cQuality_Med)
            trackBar1.Value = 2;
         else if (v == (int)eSimQuality.cQuality_Low)
            trackBar1.Value = 1;
         else if (v == (int)eSimQuality.cQuality_Min)
            trackBar1.Value = 0;

         visToSimMul = (int)Math.Pow(2, (double)eSimQuality.cQuality_Count-trackBar1.Value);

         updateLabel();
      }

      private void trackBar1_Scroll(object sender, EventArgs e)
      {
         visToSimMul = (int)Math.Pow(2, (double)eSimQuality.cQuality_Count - trackBar1.Value);

         //if (trackBar1.Value == 0)
         //   visToSimMul = (int)eSimQuality.cQuality_Min;
         //else if (trackBar1.Value == 1)
         //   visToSimMul = (int)eSimQuality.cQuality_Med;
         //else if (trackBar1.Value == 2)
         //   visToSimMul = (int)eSimQuality.cQuality_Max;

         updateLabel();
      }
   }
}