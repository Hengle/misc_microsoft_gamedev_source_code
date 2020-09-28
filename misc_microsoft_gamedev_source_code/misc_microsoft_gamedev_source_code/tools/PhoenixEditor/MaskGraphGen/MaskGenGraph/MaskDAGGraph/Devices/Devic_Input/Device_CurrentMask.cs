using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
//using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using EditorCore;

namespace graphapp
{
   public class Device_CurrentMask : MaskDevice
   {
      MaskParam mOutputMask = new MaskParam();
      [XmlIgnore]
      [ConnectionType("Output", "Primary Output")]
      public DAGMask OutputMask
      {
         get { return mOutputMask.Value; }
         set { mOutputMask.Value = value; }
      }

      public Device_CurrentMask()
      {}
      
      public Device_CurrentMask(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {

         base.Text = "Current Mask";
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

         ArrayBasedMask terrMask = (ArrayBasedMask)CoreGlobals.getEditorMain().mITerrainShared.getMask();

         int numXVerts = CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts();
         int numZVerts = CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts();
         float[] fk = new float[numXVerts * numZVerts];
         for (int x = 0; x < numXVerts; x++)
         {
            for (int y = 0; y < numZVerts; y++)
            {
               fk[x + numXVerts * y] = terrMask.GetValue(x + parms.Width * y);
            }
         }


         if (numXVerts != parms.Width || numZVerts != parms.Height)
         {
            float[] resizedHeights = ImageManipulation.resizeF32Img(fk, numXVerts, numZVerts, parms.Width, parms.Height, ImageManipulation.eFilterType.cFilter_Linear);
            fk = (float[])resizedHeights.Clone();
         }



         MaskParam mp = ((MaskParam)(connPoint.ParamType));
         mp.Value = new DAGMask(parms.Width, parms.Height);

         for (int x = 0; x < parms.Width; x++)
         {
            for (int y = 0; y < parms.Height; y++)
            {
               float k = fk[x + parms.Width* y];
               mp.Value[x, y] = k;
            }
         }

         return true;
      }

      override public string getDeviceDescriptor()
      {
         return "CurrentMask : This takes the current terrain mask as the input";
      }
      override public string getDeviceHelp()
      {
         return "CurrentMask : This description has not yet been implimented";
      }
   }
}