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
   public partial class WaterPanel : Xceed.DockingWindows.ToolWindow
   {
      public WaterPanel()
      {
         InitializeComponent();
      }

      private void WaterPanel_Load(object sender, EventArgs e)
      {
         initSliders();
      }

      private void waterBodyPaintButt_Click(object sender, EventArgs e)
      {

             
      }

      private void waterEditButt_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeWaterEdit);
      }

      private void waterOceanPaintButt_Click(object sender, EventArgs e)
      {
        

      }

      private void waterRiverButt_Click(object sender, EventArgs e)
      {
         TerrainGlobals.getEditor().setMode(BTerrainEditor.eEditorMode.cModeRiverEdit);
      }


      AdvancedHydraulicErosionFilter mErosionFilter = new AdvancedHydraulicErosionFilter();
      private void initSliders()
      {
         toolTip1.InitialDelay = 0;

         powerSilder.Setup(0,30,true);
         powerSilder.NumericValue = 1;
         powerSilder.ValueChanged += new EventHandler(powerSlider_ValueChanged);
         toolTip1.SetToolTip(powerSilder, "Scalar to how much overall erosion occurs.");

         hardnessSlider.Setup(0, 1, true);
         hardnessSlider.NumericValue = 0.3;
         hardnessSlider.ValueChanged += new EventHandler(hardnessSlider_ValueChanged);
         toolTip1.SetToolTip(hardnessSlider, "Sets the hardness of the terrain. The harder the terrain, the less likely it is to erode.\n 0=soft, 1=hard");

         dropLifeSlider.Setup(0, 64, false);
         dropLifeSlider.NumericValue = 28;
         dropLifeSlider.ValueChanged += new EventHandler(dropLifeSlider_ValueChanged);
         toolTip1.SetToolTip(hardnessSlider, "Sets the lifetime of a drop of water\n Larger lifetimes mean that water will carve longer distances.");
         
         randomSlider.Setup(0, 500, false);
         randomSlider.NumericValue = 0;
         randomSlider.ValueChanged += new EventHandler(randomSlider_ValueChanged);
         toolTip1.SetToolTip(hardnessSlider, "Sets the randomization of the erosion system");

         channelForceSlider.Setup(0, 32, false);
         channelForceSlider.NumericValue = 4;
         channelForceSlider.ValueChanged += new EventHandler(channelForceSlider_ValueChanged);
         toolTip1.SetToolTip(channelForceSlider, "An exponential scalar changing the amount of channeling occuring");

         createMaskBox.SelectedIndex = 0;
      }


      void channelForceSlider_ValueChanged(object sender, EventArgs e)
      {
         mErosionFilter.ChannelingForce = (float) (32- channelForceSlider.NumericValue);
      }
      void powerSlider_ValueChanged(object sender, EventArgs e)
      {
         mErosionFilter.Power = (float)powerSilder.NumericValue;
      }
      void hardnessSlider_ValueChanged(object sender, EventArgs e)
      {
         mErosionFilter.SoilHardness = (float)hardnessSlider.NumericValue;
      }
      void dropLifeSlider_ValueChanged(object sender, EventArgs e)
      {
         mErosionFilter.DropLife = (int)dropLifeSlider.NumericValue;
      }
      void randomSlider_ValueChanged(object sender, EventArgs e)
      {
         mErosionFilter.RandomSeed = (int)randomSlider.NumericValue;
      }
      Random mRandr = new Random();
      private void button1_Click(object sender, EventArgs e)
      {
         mErosionFilter.init();
         mErosionFilter.apply(checkBox1.Checked);

         randomSlider.NumericValue = mRandr.Next(500);
      }


      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {

      }

      private void button2_Click(object sender, EventArgs e)
      {
         initSliders();
      }



      private void createMaskBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (createMaskBox.SelectedIndex == 0) mErosionFilter.mMaskCreationType = TerrainFilter.eFilterMaskCreation.cNoMask;
         if (createMaskBox.SelectedIndex == 1) mErosionFilter.mMaskCreationType = TerrainFilter.eFilterMaskCreation.cMaskAndErosion;
         if (createMaskBox.SelectedIndex == 2) mErosionFilter.mMaskCreationType = TerrainFilter.eFilterMaskCreation.cMaskOnly;
      }

     
   
   }
}
