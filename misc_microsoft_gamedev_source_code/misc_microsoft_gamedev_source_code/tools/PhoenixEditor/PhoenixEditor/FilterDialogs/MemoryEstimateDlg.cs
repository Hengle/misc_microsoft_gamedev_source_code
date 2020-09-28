using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using ZedGraph;
using Terrain;
using Export360;

namespace PhoenixEditor.FilterDialogs
{
   public partial class MemoryEstimateDlg : Form
   {
      public MemoryEstimateDlg()
      {
         InitializeComponent();
         
      }
      private void clear()
      {
         listBox1.Items.Clear();

         zedGraphControl1.GraphPane = null;
         zedGraphControl1.GraphPane = new GraphPane();
      }
      private void refresh(bool advanced)
      {
         clear();

         if (advanced)
            CreateComponentGraph(zedGraphControl1);
         else
            CreateGraph(zedGraphControl1);

         zedGraphControl1.Invalidate();
      }
      private void MemoryEstimate_Load(object sender, EventArgs e)
      {
         refresh(false);
         timer1.Enabled = true;
         button2.Enabled = !ExportMemoryEstimate.isCalculatingMemoryEstimate();
      }

      float bytesToMB(int bytes)
      {
         return bytes / 1048576.0f;
      }

      private void CreateComponentGraph(ZedGraphControl zgc)
      {
         GraphPane myPane = zgc.GraphPane;

         if (ExportMemoryEstimate.isCalculatingMemoryEstimate())
         {
            myPane.Title.Text = "Calculating Memory... Try again soon..";

            return;
         }

         Color[] Colors = new Color[] { 
         Color.Red, Color.Green, Color.Blue, Color.Yellow, Color.Orange, Color.Teal, Color.Brown, Color.Pink, Color.Purple, Color.Tan, Color.Silver, Color.Magenta,
         Color.Maroon, Color.PeachPuff, Color.Black, Color.CornflowerBlue, Color.Cyan, Color.Khaki,Color.Azure, Color.CadetBlue, Color.Salmon, Color.Turquoise, Color.Tomato, Color.Plum};


         
         myPane.Title.Text = "XBOX Memory Estimate";


         int numMemoryElements = ExportMemoryEstimate.getNumMemoryElements();
         for (int i = 0; i < numMemoryElements; i++)
         {
            ExportMemoryEstimate.memoryElement me = ExportMemoryEstimate.getMemoryElement(i);
            myPane.AddPieSlice(me.mMemoryInBytes, Colors[i % Colors.Length], 0, me.mName);

            listBox1.Items.Add(me.mName + " : " + bytesToMB(me.mMemoryInBytes) + "MB");
         }

         listBox1.Items.Add("---------");

         int totalAvailbytes = ExportMemoryEstimate.giveTotalAvailableMemory();
         myPane.AddPieSlice(totalAvailbytes, Color.White, 0, "XBOX FREE Memory");
         listBox1.Items.Add("XBOX FREE Memory" + " : " + bytesToMB(totalAvailbytes) + "MB");
         float totalAvail = bytesToMB(totalAvailbytes);

         //this will hide the axis
         zgc.AxisChange();
      }

      private void CreateGraph(ZedGraphControl zgc)
      {
         GraphPane myPane = zgc.GraphPane;
         if (ExportMemoryEstimate.isCalculatingMemoryEstimate())
         {
            myPane.Title.Text = "Calculating Memory... Try again soon..";

            return;
         }


         myPane.Title.Text = "XBOX Memory Estimate";

         int totalMBinBytes = ExportMemoryEstimate.giveTotalMemoryUsage();
         int totalAvailinBytes = ExportMemoryEstimate.giveTotalAvailableMemory();

         float totalMB = bytesToMB(totalMBinBytes);
         float totalAvail = bytesToMB(totalAvailinBytes);


         myPane.AddPieSlice(totalMBinBytes, Color.Red, 0, "Used Memory");
         myPane.AddPieSlice(totalAvailinBytes, Color.White, 0, "Available Memory");


         listBox1.Items.Add("Used Memory : " + totalMB + "MB");
         listBox1.Items.Add("Available Memory : " + totalAvail + "MB");
      
         //this will hide the axis
         zgc.AxisChange();
      }

      private void button1_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      private void button2_Click(object sender, EventArgs e)
      {
         button2.Enabled = false;
         TerrainGlobals.getTerrainFrontEnd().updateMemoryEstimate(true,false);
         refresh(false);
         button2.Enabled = true;
      }

      private void timer1_Tick(object sender, EventArgs e)
      {
         button2.Enabled = !ExportMemoryEstimate.isCalculatingMemoryEstimate();
      }

      private void button3_Click(object sender, EventArgs e)
      {
         if(button3.Text == "Basic View")
         {
            button3.Text = "Advanced View";
            refresh(false);
         }
         else if(button3.Text == "Advanced View")
         {
            button3.Text = "Basic View";
            refresh(true);
         }
      }
   }
}