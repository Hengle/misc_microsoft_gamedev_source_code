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

namespace graphapp
{
   public class Device_RandomClampedFloat : MaskDevice
   {
      FloatParam mOutputFloat = new FloatParam();
      [XmlIgnore]
      [ConnectionType("Output", "Primary Output")]
      public float OutputScalar
      {
         get { return mOutputFloat.Value; }
         set { mOutputFloat.Value = value; }
      }

      [XmlIgnore]
      Random mRandVal = new Random(DateTime.Now.Millisecond);

      public Device_RandomClampedFloat()
      { }
      public Device_RandomClampedFloat(GraphCanvas owningCanvas)
         :
          base(owningCanvas)
      {
         base.Text = "Random Clamped Float";
         mColorTop = Color.White;
         mColorBottom = Color.Yellow;
         mBorderSize = 1;

         mSize.Width = 60;
         mSize.Height = 20;

         generateConnectionPoints();
         resizeFromConnections();
      }
      public override bool load(MaskDAGGraphNode fromNode)
      {
         Device_RandomClampedFloat dc = fromNode as Device_RandomClampedFloat;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         OutputScalar = dc.OutputScalar;

         return true;
      }
      override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
         if (!verifyInputConnections())
            return false;

         if (!gatherInputAndParameters(parms))
            return false;

         if (!(connPoint.ParamType is FloatParam))
            return false;

         FloatParam mp = ((FloatParam)(connPoint.ParamType));


         mp.Value = ((float)mRandVal.NextDouble());

         return true;
      }

   }
}