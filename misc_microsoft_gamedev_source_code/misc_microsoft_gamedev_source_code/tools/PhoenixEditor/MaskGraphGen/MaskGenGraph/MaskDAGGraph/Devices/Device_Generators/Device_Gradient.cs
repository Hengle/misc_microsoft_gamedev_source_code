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




   public class Device_Gradient : MaskDevice
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

      IntParam mDirection = new IntParam(0, 0, 360);
      [ConnectionType("Param", "Direction")]
      public int Direction
      {
         get { return mDirection.Value; }
         set { mDirection.Value = value; }
      }

     



      public Device_Gradient()
      {}
      public Device_Gradient(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {

         base.Text = "Gradient";
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
         Device_Gradient dc = fromNode as Device_Gradient;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         Direction = dc.Direction;

         return true;
      }
      override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
         if (!verifyInputConnections())
            return false;

         gatherInputAndParameters(parms);


         System.Drawing.Drawing2D.Matrix mTransformMat = new Matrix();
         mTransformMat.Translate(-(parms.Width >> 1), -(parms.Height >> 1));
         mTransformMat.Rotate(Direction, MatrixOrder.Append);
         mTransformMat.Translate((parms.Width >> 1), (parms.Height >> 1), MatrixOrder.Append);


         MaskParam mp = ((MaskParam)(connPoint.ParamType));
         mp.Value = new DAGMask(parms.Width, parms.Height);
         mp.Value.mConstraintMask = ConstraintMask;

         Point[] pts = new Point[1];
         

         float incStep = 1.0f / parms.Width;
         for (int y = 0; y < parms.Height; y++)
         {
            for (int x = 0; x < parms.Width; x++)
            {
               pts[0].X = x;
               pts[0].Y = y;
               mTransformMat.TransformPoints(pts);   

               mp.Value[x, y] = incStep * pts[0].X;
            }
         }
         


         return true;
      }


   }
}