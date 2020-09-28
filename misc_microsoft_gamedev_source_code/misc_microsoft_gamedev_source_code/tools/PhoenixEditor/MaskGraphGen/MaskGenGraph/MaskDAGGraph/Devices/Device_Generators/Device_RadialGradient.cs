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
using System.Xml;
using System.Xml.Serialization;

using EditorCore;

namespace graphapp
{




   public class Device_RadialGradient : MaskDevice
   {
      MaskParam mConstraintMask = new MaskParam();
      [XmlIgnore]
      [ConnectionType("Constraint", "Primary Constraint")]
      public DAGMask ConstraintMask
      {
         get { return mConstraintMask.Value; }
         set { mConstraintMask.Value = value; }
      }

      MaskParam mOutputMask = new MaskParam();
      [XmlIgnore]
      [ConnectionType("Output", "Primary Output")]
      public DAGMask OutputMask
      {
         get { return mOutputMask.Value; }
         set { mOutputMask.Value = value; }
      }

      FloatParam mRadius = new FloatParam(0.5f, 0, 1);
      [ConnectionType("Param", "Radius")]
      public float Radius
      {
         get { return mRadius.Value; }
         set { mRadius.Value = value; }
      }

      
      FloatParam mHotSpot = new FloatParam(0.5f, 0, 1);
      [ConnectionType("Param", "HotSpot")]
      public float HotSpot
      {
         get { return mHotSpot.Value; }
         set { mHotSpot.Value = value; }
      }

      FloatParam mIntensity = new FloatParam(0.5f, 0, 1);
      [ConnectionType("Param", "Intensity")]
      public float Intensity
      {
         get { return mIntensity.Value; }
         set { mIntensity.Value = value; }
      }





      public Device_RadialGradient()
      { }
      public Device_RadialGradient(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {

         base.Text = "Radial Gradient";
         mColorTop = Color.White;
         mColorBottom = Color.Green;
         mBorderSize = 1;

         mSize.Width = 60;
         mSize.Height = 20;

         generateConnectionPoints();
         resizeFromConnections();
      }
      public override bool load(MaskDAGGraphNode fromNode)
      {
         Device_RadialGradient dc = fromNode as Device_RadialGradient;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         Radius = dc.Radius;
         Intensity = dc.Intensity;
         HotSpot = dc.HotSpot;

         return true;
      }
      override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
         if (!verifyInputConnections())
            return false;

         gatherInputAndParameters(parms);

         

         MaskParam mp = ((MaskParam)(connPoint.ParamType));
         mp.Value = new DAGMask(parms.Width, parms.Height);
         mp.Value.mConstraintMask = ConstraintMask;


         int xOffset = parms.Width >> 1;
         int yOffset = parms.Height >> 1;

         int radius = (int)(Radius * parms.Width);
         int hotRadius = (int)(radius * HotSpot);
         int diff = (int)(radius - hotRadius);
         int mnX = Math.Max(0, xOffset - radius);
         int mnY = Math.Max(0, yOffset - radius);
         int mxX = Math.Min(parms.Width - 1, xOffset + radius);
         int mxY = Math.Min(parms.Height - 1, yOffset + radius);

         for (int i = mnX; i < mxX; i++)
         {
            for (int j = mnY; j < mxY; j++)
            {
               float gradVal = 0;

               float dist = (float)Math.Sqrt((xOffset - i) * (xOffset - i) + (yOffset - j) * (yOffset - j));
               if (dist < radius)
               {
                  if (dist < hotRadius)
                  {
                     gradVal = 1.0f;
                  }
                  else
                  {
                     float gradDiff = 1 - ((dist - (hotRadius)) / (float)(diff));

                     gradVal = gradDiff;
                  }
               }

               if (gradVal == 0)
                  continue;

              
               mp.Value[i, j] = Intensity * gradVal;
            }
         }
      

         return true;
      }


   }
}