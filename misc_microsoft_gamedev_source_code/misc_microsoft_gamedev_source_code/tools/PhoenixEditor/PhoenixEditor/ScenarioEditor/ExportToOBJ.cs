using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using EditorCore;
using Terrain;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ExportToOBJ : Form
   {
      public ExportToOBJ()
      {
         InitializeComponent();
      }

      private void excludeSimVerts_CheckedChanged(object sender, EventArgs e)
      {

      }

      private void button1_Click(object sender, EventArgs e)
      {
         SaveFileDialog d = new SaveFileDialog();
         d.Filter = "OBJ file (*.obj)|*.obj";
         d.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
         if (d.ShowDialog() == DialogResult.OK)
         {
            textBox1.Text = d.FileName;
         }
      }

      private void button2_Click(object sender, EventArgs e)
      {
         float eLOD = 0.25f;
         if (highMeshLOD.Checked) eLOD = 0.25f;
         if (medMeshLOD.Checked) eLOD = 0.5f;
         if (lowMeshLOD.Checked) eLOD = 1.0f;
         string outputFile = textBox1.Text;

         if(exportAllVerts.Checked)
         {
            if (refineRQT.Checked)
            {
               TerrainGlobals.getTerrainFrontEnd().exportRefinedRQTTerrainToObj(outputFile, exportSelectedVerts.Checked );
            }
            else if(refineTIN.Checked)
            {
               TerrainGlobals.getTerrainFrontEnd().exportRefinedTINTerrainToObj(outputFile, exportSelectedVerts.Checked ,eLOD);
            }
            else if(refineNone.Checked)
            {
               ObjExporter exp = new ObjExporter();
               exp.writeAll(outputFile);
               exp = null;
            } 
            
         }
         else if (exportSelectedVerts.Checked)
         {
            if (refineRQT.Checked)
            {
               TerrainGlobals.getTerrainFrontEnd().exportRefinedRQTTerrainToObj(outputFile, exportSelectedVerts.Checked);
            }
            else if (refineTIN.Checked)
            {
               TerrainGlobals.getTerrainFrontEnd().exportRefinedTINTerrainToObj(outputFile, exportSelectedVerts.Checked, eLOD);
            }
            else if(refineNone.Checked)
            {
               ObjExporter exp = new ObjExporter();
               exp.writeSelection(outputFile);
               exp = null;
            }
         }
         Close();
      }

      private void button3_Click(object sender, EventArgs e)
      {
         Close();
      }

      private void exportAllVerts_CheckedChanged(object sender, EventArgs e)
      {

      }

      private void exportSelectedVerts_CheckedChanged(object sender, EventArgs e)
      {
        
      }


      private void refineTIN_CheckedChanged(object sender, EventArgs e)
      {
         
      }

      private void refineRQT_CheckedChanged(object sender, EventArgs e)
      {
         
      }

      private void refineNone_CheckedChanged(object sender, EventArgs e)
      {
         
      }
   }
}