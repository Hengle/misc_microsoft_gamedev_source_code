using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using EditorCore;
using Terrain;
using SimEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ScenarioProperties : UserControl
   {
      public ScenarioProperties()
      {
         InitializeComponent();
       
      }
      public void scenarioDataChanged()
      {
         initSliders();
         dataToControls();
      }

      private void dataToControls()
      {
         scenarioFileNameBox.Text = SimGlobals.getSimProperties().mScenarioFilename;
         rootTerrainNameBox.Text = SimGlobals.getSimProperties().mRootTerrainFile;
         displaynameNameBox.Text = SimGlobals.getSimProperties().mScenarioDisplayname;
      }

      private void initSliders()
      {
         boundryTopSlider.Setup(0, TerrainGlobals.getTerrain().getNumXVerts(), false);
         boundryTopSlider.NumericValue = SimGlobals.getSimProperties().mTopBoundry;
         boundryTopSlider.ValueChanged += new EventHandler(boundryTopSlider_ValueChanged);

         boundryBottomSlider.Setup(0, TerrainGlobals.getTerrain().getNumXVerts(), false);
         boundryBottomSlider.NumericValue = SimGlobals.getSimProperties().mBottomBoundry;
         boundryBottomSlider.ValueChanged += new EventHandler(boundryBottomSlider_ValueChanged);

         boundryLeftSlider.Setup(0, TerrainGlobals.getTerrain().getNumXVerts(), false);
         boundryLeftSlider.NumericValue = SimGlobals.getSimProperties().mLeftBoundry;
         boundryLeftSlider.ValueChanged += new EventHandler(boundryLeftSlider_ValueChanged);

         boundryRightSlider.Setup(0, TerrainGlobals.getTerrain().getNumXVerts(), false);
         boundryRightSlider.NumericValue = SimGlobals.getSimProperties().mRightBoundry;
         boundryRightSlider.ValueChanged += new EventHandler(boundryRightSlider_ValueChanged);
      }

      void boundryRightSlider_ValueChanged(object sender, EventArgs e)
      {
         SimGlobals.getSimProperties().mRightBoundry = (int)boundryRightSlider.NumericValue;
      }
      void boundryLeftSlider_ValueChanged(object sender, EventArgs e)
      {
         SimGlobals.getSimProperties().mLeftBoundry = (int)boundryLeftSlider.NumericValue;
      }
      void boundryBottomSlider_ValueChanged(object sender, EventArgs e)
      {
         SimGlobals.getSimProperties().mBottomBoundry = (int)boundryBottomSlider.NumericValue;
      }
      void boundryTopSlider_ValueChanged(object sender, EventArgs e)
      {
         SimGlobals.getSimProperties().mTopBoundry = (int)boundryTopSlider.NumericValue;
      }

      private void ScenarioProperties_Load(object sender, EventArgs e)
      {
      
      }
   }
}
