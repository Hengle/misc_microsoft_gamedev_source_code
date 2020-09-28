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
   public partial class SimObjAdvNumericRotate : Form
   { 
      public SimObjAdvNumericRotate()
      {
         InitializeComponent();
          
         
      }

      private void button1_Click(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().rotateObjectsExplicit(  Geometry.DegreeToRadian((float)xValue.NumericValue),
                                                         Geometry.DegreeToRadian((float)yValue.NumericValue),
                                                         Geometry.DegreeToRadian((float)zValue.NumericValue),
                                                         false,
                                                         checkBox1.Checked);
      }

      private void SimObjAdvNumericMove_Load(object sender, EventArgs e)
      {
         Vector3 minBounds = CoreGlobals.getEditorMain().mITerrainShared.getBBMin();
         Vector3 maxBounds = CoreGlobals.getEditorMain().mITerrainShared.getBBMax();

         Vector3 size = maxBounds - minBounds;

         xValue.Setup(-360, 360, true);
         yValue.Setup(-360, 360, true);
         zValue.Setup(-360, 360, true);

         xValue.NumericValue = 0;
         yValue.NumericValue = 0;
         zValue.NumericValue = 0;
      }

      private void SimObjAdvNumericMove_Activated(object sender, EventArgs e)
      {
         
      }
   }
}