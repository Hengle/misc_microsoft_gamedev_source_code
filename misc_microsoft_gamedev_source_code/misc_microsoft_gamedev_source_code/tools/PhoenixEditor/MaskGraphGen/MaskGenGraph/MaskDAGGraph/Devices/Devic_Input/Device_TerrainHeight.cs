using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using Microsoft.DirectX;

using EditorCore;

namespace graphapp
{
   public class Device_TerrainHeight : MaskDevice
   {
      MaskParam mOutputMask = new MaskParam();
      [XmlIgnore]
      [ConnectionType("Output", "Primary Output")]
      public DAGMask OutputMask
      {
         get { return mOutputMask.Value; }
         set { mOutputMask.Value = value; }
      }

      public Device_TerrainHeight()
      {}

      public Device_TerrainHeight(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {

         base.Text = "Terrain Height";
         mColorTop = Color.White;
         mColorBottom = Color.HotPink;
         mBorderSize = 1;

         mSize.Width = 60;
         mSize.Height = 20;


         generateConnectionPoints();
         resizeFromConnections();
      }


      override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
         if (!verifyInputConnections())
            return false;

         gatherInputAndParameters(parms);


         MaskParam mp = ((MaskParam)(connPoint.ParamType));
         mp.Value = new DAGMask(parms.Width, parms.Height);

         float mx = CoreGlobals.getEditorMain().mITerrainShared.getBBMax().Y;
         float mn = CoreGlobals.getEditorMain().mITerrainShared.getBBMin().Y;
         float delta = mx-mn;

         int numXVerts = CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts();
         int numZVerts = CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts();
         float[] fk = new float[numXVerts * numZVerts];
         for (int x = 0; x < numXVerts; x++)
         {
            for (int y = 0; y < numZVerts; y++)
            {
               fk[x + numXVerts * y] = CoreGlobals.getEditorMain().mITerrainShared.getTerrainHeight(x, y);
            }
         }


         if (numXVerts != parms.Width || numZVerts != parms.Height)
         {
            float[] resizedHeights = ImageManipulation.resizeF32Img(fk,numXVerts,numZVerts,parms.Width,parms.Height, ImageManipulation.eFilterType.cFilter_Linear);
            fk = (float[])resizedHeights.Clone();
         }


         for (int x = 0; x < parms.Width; x++)
         {
            for (int y = 0; y < parms.Height; y++)
            {

               float k = fk[x+parms.Width* y];
               k -= mn;
               k /= delta;

               mp.Value[x, y] = k;
            }
         }

         return true;
      }

      override public string getDeviceDescriptor()
      {
         return "TerrainHeight : This takes the current terrain height as the input";
      }
      override public string getDeviceHelp()
      {
         return "TerrainHeight : This description has not yet been implimented";
      }

   }
}