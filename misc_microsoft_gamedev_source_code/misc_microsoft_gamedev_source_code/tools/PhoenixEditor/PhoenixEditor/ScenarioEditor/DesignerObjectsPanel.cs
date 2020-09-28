using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using SimEditor;
using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class DesignerObjectsPanel : Xceed.DockingWindows.ToolWindow //UserControl
   {
      public DesignerObjectsPanel()
      {
         InitializeComponent();
         this.Text = "AI";
      }

      private void PlaceSphereButton_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().PlaceGameDesignValueSphere();
      }

      private void PlacePathStartButton_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().PlaceGameDesignLine();

      }

      private void EditLinesButton_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().EditSelectedLines();
      }

      private void AddPointsButton_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().PlaceGameDesignLinePoints();

      }

      private void PlacePointButton_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().PlaceGameDesignValuePoint();
      }

      private void InsertPointButton_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().InsertGameDesignLinePoint();
      }

      private void CloseLineLoopButton_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().CloseGameDesignLine();
      }
   }
}
