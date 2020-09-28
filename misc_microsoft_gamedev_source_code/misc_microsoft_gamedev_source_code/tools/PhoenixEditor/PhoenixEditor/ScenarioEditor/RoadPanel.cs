using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Xceed.DockingWindows;
using System.IO;
using SimEditor;
using Terrain;
using EditorCore;
using Rendering;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class RoadPanel : Xceed.DockingWindows.ToolWindow
   {

      int mSelectedRoadTextue = -1;
      string mSelectedRoadTextureName = null;
      string[] giveValidRoadTextures()
      {
         return Directory.GetFiles(CoreGlobals.getWorkPaths().mRoadsPath, "*_df.ddx");
      }
      static public string getPureFileNameNoExt(string fname)
      {
         string[] stripList = new string[5] { "_df", "_nm", "_sp", "_em", "_rm" };

         for (int i = 0; i < 5; i++)
         {
            int k = fname.IndexOf(stripList[i]);
            if (k != -1)

               return fname.Substring(0, fname.LastIndexOf("_"));
         }


         return fname;
      }

      void populateListBox()
      {
         string[] roadTexures = giveValidRoadTextures();
         for (int i = 0; i < roadTexures.Length; i++)
         {
            //strip out _df
            roadTextureListBox.Items.Add(getPureFileNameNoExt(Path.GetFileNameWithoutExtension(roadTexures[i])));
         }
      }


      //---------------------------------------
      public RoadPanel()
      {
         InitializeComponent();
      }
      //---------------------------------------
      public void postSelectionCallback()
      {
         int numroadsSelected = RoadManager.getNumSelectedRoads();
         if (numroadsSelected > 1 || RoadManager.getSelectedRoad(0)==null)
         {
            groupBox4.Enabled = false;
         }
         else
         {
            groupBox4.Enabled = true;
            roadWidthSlider.mNumericValue = RoadManager.getSelectedRoad(0).getRoadWidth();
         }

         int numPointsSelected = RoadManager.getNumSelectedPoints();
         if(numPointsSelected ==0)
         {
            groupBox3.Enabled = false;
         }
         else
         {
            groupBox3.Enabled = true;
            if(numPointsSelected>1)
            {
               roadNodeType.SelectedIndex = -1;
            }
            else
            {
               roadControlPoint.eControlPointType targetType = RoadManager.getSelectedRoad(0).getSelectedControlPoint(0).mControlPointType;
               if (targetType == roadControlPoint.eControlPointType.cAngled) roadNodeType.SelectedIndex = 0;
              // if (targetType == roadControlPoint.eControlPointType.cTightCurve) roadNodeType.SelectedIndex = 1;
               if (targetType == roadControlPoint.eControlPointType.cSplinePoint) roadNodeType.SelectedIndex = 1;
            }
         }
      }
      //---------------------------------------
      private void RoadPanel_Load(object sender, EventArgs e)
      {
         populateListBox();
      //   roadTextureListBox.SelectedIndex = 0;
         comboBox2.SelectedIndex = 0;

         roadWidthSlider.Setup(0.5, 4, true);
         roadWidthSlider.NumericValue = 1;
         roadWidthSlider.ValueChanged += new EventHandler(roadWidthSlider_ValueChanged);

         tesselationSlider.Setup(0.25, 1, true);
         tesselationSlider.NumericValue = 0.75;
         tesselationSlider.ValueChanged += new EventHandler(tesselationSlider_ValueChanged);
      }

      void tesselationSlider_ValueChanged(object sender, EventArgs e)
      {
         float f = (float)((tesselationSlider.NumericValue));
         RoadManager.changeSelectedRoadWorldTesselation(f);
         RoadManager.rebuildSelectedRoadVisuals();
      }

      void roadWidthSlider_ValueChanged(object sender, EventArgs e)
      {
         RoadManager.changeSelectedRoadWidth((float)roadWidthSlider.NumericValue);
         RoadManager.rebuildSelectedRoadVisuals();
      }

      private void roadTextureListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         mSelectedRoadTextue = roadTextureListBox.SelectedIndex;
         mSelectedRoadTextureName = (string)roadTextureListBox.SelectedItem;
         RoadManager.setRoadTexture(mSelectedRoadTextureName);

         tabControl1.SelectedTab = tabControl1.TabPages[0];
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeRoadAdd);
      }
       

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         RoadManager.setContinuousCreationMode(checkBox1.Checked);
      }

      private void tabControl1_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (tabControl1.SelectedIndex == 0)
         {
            if (roadTextureListBox.SelectedIndex != -1)
            {
               TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeRoadAdd);
               comboBox2.SelectedIndex = 0;
            }
         }
         else if (tabControl1.SelectedIndex == 1)
         {
            TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeRoadEdit);
         }
      }

      private void tabControl1_TabIndexChanged(object sender, EventArgs e)
      {
         if (tabControl1.TabIndex == 0)
         {
            if (roadTextureListBox.SelectedIndex != -1)
               TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeRoadAdd);
         }
         else if (tabControl1.TabIndex == 1)
         {
            TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeRoadEdit);
         }
      }

      private void comboBox2_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (comboBox2.SelectedIndex == 0) RoadManager.setCurrentAddModeType(roadControlPoint.eControlPointType.cAngled);
        // if (comboBox2.SelectedIndex == 1) RoadManager.setCurrentAddModeType(roadControlPoint.eControlPointType.cTightCurve);
         if (comboBox2.SelectedIndex == 1) RoadManager.setCurrentAddModeType(roadControlPoint.eControlPointType.cSplinePoint);
      }

      private void comboBox3_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (roadNodeType.SelectedIndex == -1)
            return;

         roadControlPoint.eControlPointType targetType = roadControlPoint.eControlPointType.cAngled;
         if (roadNodeType.SelectedIndex == 0) targetType=(roadControlPoint.eControlPointType.cAngled);
      //   if (roadNodeType.SelectedIndex == 1) targetType=(roadControlPoint.eControlPointType.cTightCurve);
         if (roadNodeType.SelectedIndex == 1) targetType=(roadControlPoint.eControlPointType.cSplinePoint);

         RoadManager.changeSelectedPointType(targetType);

      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         RoadManager.setSnapToPrimeMode(checkBox2.Checked);
      }

      private void button1_Click(object sender, EventArgs e)
      {
         RoadManager.splitSelectedPoints();
      }

      private void button2_Click(object sender, EventArgs e)
      {
         RoadManager.collapseSelectedPoints();
      }

      private void button3_Click(object sender, EventArgs e)
      {
         RoadManager.snapSelectedRoadToTerrain();
      }

      private void button5_Click(object sender, EventArgs e)
      {
         RoadManager.smoothSelectedRoadTerrain();
      }

      private void button4_Click(object sender, EventArgs e)
      {
         RoadManager.maskSelectedRoadTerrain();
      }


     
   }
}
