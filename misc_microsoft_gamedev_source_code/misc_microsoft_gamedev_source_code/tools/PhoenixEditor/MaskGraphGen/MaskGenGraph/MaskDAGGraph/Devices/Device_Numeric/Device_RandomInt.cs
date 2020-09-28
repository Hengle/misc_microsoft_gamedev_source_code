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
   public class Device_RandomInt : MaskDevice
   {
      IntParam mOutputFloat = new IntParam();
      [XmlIgnore]
      [ConnectionType("Output", "Primary Output")]
      public int OutputScalar
      {
         get { return mOutputFloat.Value; }
         set { mOutputFloat.Value = value; }
      }

      [XmlIgnore]
      Random mRandVal = new Random(DateTime.Now.Millisecond);

      public Device_RandomInt()
      { }
      public Device_RandomInt(GraphCanvas owningCanvas)
         :
          base(owningCanvas)
      {
         base.Text = "Random Integer";
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
         Device_RandomInt dc = fromNode as Device_RandomInt;
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

         if (!(connPoint.ParamType is IntParam))
            return false;

         IntParam mp = ((IntParam)(connPoint.ParamType));

         mp.Value = mRandVal.Next();

         return true;
      }

   }
}