using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using ZedGraph;

using Terrain;

namespace PhoenixEditor
{
   public partial class ExporterResults : Form
   {
      public ExporterResults()
      {
         InitializeComponent();

      }

      public Export360.ExportResults results = new Export360.ExportResults();

      float bytesToMB(int bytes)
      {
         return bytes / 1048576.0f;
      }
      private void ExporterResults_Load(object sender, EventArgs e)
      {
         CreateGraph(zg1);

         listBox1.Items.Add("XTD FILE");
         listBox1.Items.Add("    Positions : " + bytesToMB(results.terrainPositionMemorySize) + "MB");
         listBox1.Items.Add("    Normals : " + bytesToMB(results.terrainNormalsMemorySize) + "MB");
         listBox1.Items.Add("    Alpha : " + bytesToMB(results.terrainAlphaMemorySize) + "MB");
         listBox1.Items.Add("    Tesselation : " + bytesToMB(results.terrainTessValuesMemorySize) + "MB");
         listBox1.Items.Add("    AO : " + bytesToMB(results.terrainAOMemorySize) + "MB");
         listBox1.Items.Add("       Time: " + results.terrainAOTime + "Min");
         listBox1.Items.Add("    Lighting : " + bytesToMB(results.terrainLightingMemorySize) + "MB");
         listBox1.Items.Add("       Time: " + results.terrainLightingTime + "Min");

         listBox1.Items.Add("XTT FILE-----------");
         listBox1.Items.Add("    Texturing : " + bytesToMB(results.terrainTextureMemorySize) + "MB");
         listBox1.Items.Add("      Time: " + results.terrainTextureTime + "Min");
         listBox1.Items.Add("    Unique Texturing : " + bytesToMB(results.terrainUniqueTextureMemorySize) + "MB");
         listBox1.Items.Add("      Time: " + results.terrainUniqueTextureTime + "Min");
         listBox1.Items.Add("    Roads : " + bytesToMB(results.terrainRoadMemory) + "MB");
         listBox1.Items.Add("      Time: " + results.terrainRoadTime + "Min");

         listBox1.Items.Add("XTH FILE-----------");
         listBox1.Items.Add("    GPU Heights : " + bytesToMB(results.terrainGPUHeighfieldMemory) + "MB");
         listBox1.Items.Add("      Time: " + results.terrainGPUHeighfieldTime + "Min");


         listBox1.Items.Add("XSD FILE-----------");
         listBox1.Items.Add("    Sim Terrain : " + bytesToMB(results.terrainSimMemorySize) + "MB");
         listBox1.Items.Add("      Time: " + results.terrainSimTime + "Min");

       
         listBox1.Items.Add("-------------");

         int total = results.terrainPositionMemorySize + results.terrainNormalsMemorySize + results.terrainAOMemorySize +
            results.terrainTessValuesMemorySize + results.terrainTextureMemorySize + results.terrainSimMemorySize + results.terrainAlphaMemorySize +
            results.terrainLightingMemorySize + results.terrainUniqueTextureMemorySize + results.terrainRoadMemory + results.terrainGPUHeighfieldMemory;
         listBox1.Items.Add("Total : " + bytesToMB(total) + "MB");

       //  int min = results.totalTime / 60;
       //  int sec = results.totalTime % 60;
         listBox1.Items.Add("Total Time: " + results.totalTime + "Min");
      }

      private void button1_Click(object sender, EventArgs e)
      {
         this.Close();
         results = null;
      }

      private void CreateGraph(ZedGraphControl zgc)
      {

         int total = results.getTotalMemory();


         GraphPane myPane = zgc.GraphPane;
         myPane.Title.Text = "XBOX Terrain Memory Footprint";



         //PieItem pi = 
         myPane.AddPieSlice(results.terrainPositionMemorySize, Color.Red, 0, "Positions");
         myPane.AddPieSlice(results.terrainNormalsMemorySize, Color.Orange, 0, "Normals");
         myPane.AddPieSlice(results.terrainAlphaMemorySize, Color.Yellow, 0, "Alpha");
         myPane.AddPieSlice(results.terrainTessValuesMemorySize, Color.Blue, 0, "Tesselation");
         myPane.AddPieSlice(results.terrainAOMemorySize, Color.Green, 0, "AO");
         myPane.AddPieSlice(results.terrainLightingMemorySize , Color.Teal, 0, "Lighting");
         myPane.AddPieSlice(results.terrainTextureMemorySize, Color.Black, 0, "Texturing Blend Textures");
         myPane.AddPieSlice(results.terrainUniqueTextureMemorySize , Color.White, 0, "Unique Texture");
         myPane.AddPieSlice(results.terrainRoadMemory, Color.Pink, 0, "Roads");
         myPane.AddPieSlice(results.terrainFoliageMemory , Color.Purple, 0, "Foliage");
         myPane.AddPieSlice(results.terrainGPUHeighfieldMemory , Color.Silver, 0, "Decal Heights");
         myPane.AddPieSlice(results.terrainSimMemorySize , Color.Tan, 0, "Sim Data");

         float totalMB = bytesToMB(total);

         // Make a text label to highlight the total value
         TextObj text = new TextObj("Terrain Total Memory\n" + totalMB.ToString() + "MB",0.1F, 0.90F, CoordType.PaneFraction);
         text.Location.AlignH = AlignH.Center;
         text.Location.AlignV = AlignV.Bottom;
         text.FontSpec.Border.IsVisible = false;
         text.FontSpec.Fill = new Fill(Color.White, Color.FromArgb(255, 255, 255), 45F);
         text.FontSpec.StringAlignment = StringAlignment.Center;
         myPane.GraphObjList.Add(text);

         //this will hide the axis
         zgc.AxisChange();
        
      }

      private void button2_Click(object sender, EventArgs e)
      {
         if (this.Width == 866)
         {
            this.Width = 576;
            button2.Text = ">>";
         }
         else if (this.Width == 576)
         {
            this.Width = 866;
            button2.Text = "<<";
         }
      }
   }
}