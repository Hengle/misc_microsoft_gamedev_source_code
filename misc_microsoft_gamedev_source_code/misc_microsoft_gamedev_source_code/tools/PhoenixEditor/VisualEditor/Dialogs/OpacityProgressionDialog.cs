using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace VisualEditor.Dialogs
{
   public partial class OpacityProgressionDialog : Form
   {
      private bool bInitialized = false;


      public OpacityProgressionDialog()
      {
         InitializeComponent();
      }

      public void setData(EditorCore.FloatProgression progression)
      {
         scalarProgressionControl.AxisMinY = 0;
         scalarProgressionControl.AxisMaxY = 100;
         scalarProgressionControl.AxisMinX = 0;
         scalarProgressionControl.AxisMaxX = 100;
         scalarProgressionControl.ChartStartColor = Color.Blue;
         scalarProgressionControl.ChartEndColor = Color.Cyan;
         scalarProgressionControl.LoopControl = true;
         scalarProgressionControl.ProgressionName = "Opacity";

         scalarProgressionControl.setData(progression);

         bInitialized = true;
      }
   }
}