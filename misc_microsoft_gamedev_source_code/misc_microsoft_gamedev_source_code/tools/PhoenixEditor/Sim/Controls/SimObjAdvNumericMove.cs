using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using EditorCore;

namespace SimEditor.Controls
{
   public partial class SimObjAdvNumericMove : Form
   {
      public SimObjAdvNumericMove()
      {
         InitializeComponent();

         
      }

      private void button1_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().moveObjectsExplicit(    (float)xValue.NumericValue,
                                                         (float)yValue.NumericValue,
                                                         (float)zValue.NumericValue,
                                                         false);
      }

      private void SimObjAdvNumericMove_Load(object sender, EventArgs e)
      {
         Vector3 minBounds = CoreGlobals.getEditorMain().mITerrainShared.getBBMin();
         Vector3 maxBounds = CoreGlobals.getEditorMain().mITerrainShared.getBBMax();

         Vector3 size = maxBounds - minBounds;

         xValue.Setup(-size.X, size.X, true);
         yValue.Setup(-size.Y, size.Y, true);
         zValue.Setup(-size.Z, size.Z, true);

         Vector3 widgetPos = SimGlobals.getSimMain().giveWidgetPos();
         xValue.NumericValue = widgetPos.X;
         yValue.NumericValue = widgetPos.Y;
         zValue.NumericValue = widgetPos.Z;
      }


      private void SimObjAdvNumericMove_Activated(object sender, EventArgs e)
      {
         Vector3 widgetPos = SimGlobals.getSimMain().giveWidgetPos();
         xValue.NumericValue = widgetPos.X;
         yValue.NumericValue = widgetPos.Y;
         zValue.NumericValue = widgetPos.Z;
      }
   }
}