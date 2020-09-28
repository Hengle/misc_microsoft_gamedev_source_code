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
   public class Device_DistanceTransform : MaskDevice
   {
      MaskParam mConstraintMask = new MaskParam();
      [XmlIgnore]
      [ConnectionType("Constraint", "Primary Constraint")]
      public DAGMask ConstraintMask
      {
         get { return mConstraintMask.Value; }
         set { mConstraintMask.Value = value; }
      }

      MaskParam mInputMask = new MaskParam();
      [XmlIgnore]
      [ConnectionType("Input", "Primary Input", true)]
      public DAGMask InputMask
      {
         get { return mInputMask.Value; }
         set { mInputMask.Value = value; }
      }

      MaskParam mOutputMask = new MaskParam();
      [XmlIgnore]
      [ConnectionType("Output", "Primary Output")]
      public DAGMask OutputMask
      {
         get { return mOutputMask.Value; }
         set { mOutputMask.Value = value; }
      }



      public Device_DistanceTransform()
      { }
      public Device_DistanceTransform(GraphCanvas owningCanvas)
         :
          base(owningCanvas)
      {
         base.Text = "Distance Transform";
         mColorTop = Color.White;
         mColorBottom = Color.CornflowerBlue;
         mBorderSize = 1;

         mSize.Width = 60;
         mSize.Height = 20;

         generateConnectionPoints();
         resizeFromConnections();
      }
      public override bool load(MaskDAGGraphNode fromNode)
      {
         Device_DistanceTransform dc = fromNode as Device_DistanceTransform;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         return true;
      }
      override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
         if (!verifyInputConnections())
            return false;

         if (!gatherInputAndParameters(parms))
            return false;

         MaskParam mp = ((MaskParam)(connPoint.ParamType));
         mp.Value = InputMask.Clone();
         mp.Value.mConstraintMask = ConstraintMask;

         int numTimesToRun = parms.Width >> 1;
         DAGMask outMask = InputMask.Clone();// 

         DAGMask distMask = new DAGMask(parms.Width, parms.Height);

         int maxRange = 0;
         for (int i = 0; i < numTimesToRun;i++ )
         {
            DAGMask smallerMask = new DAGMask(parms.Width, parms.Height);
            bool madeChange = contractSetValue(ref outMask, ref smallerMask, ref distMask, i);
               
            outMask = smallerMask.Clone();
            if (madeChange)
               maxRange = i;
            else
               break;
         }


         //normalize our input now...
         for (int x = 0; x < parms.Width; x++)
         {
            for (int y = 0; y < parms.Height; y++)
            {
               mp.Value[x, y] = distMask[x, y] / (float)maxRange;
            }
         }

         outMask = null;

         return true;
      }

      bool contractSetValue(ref DAGMask src, ref DAGMask dst, ref DAGMask distMask,float valToSet)
      {
         Point[] neighOffset = new Point[]{new Point(-1,-1),new Point(-1,0),new Point(-1,1),
                                        new Point(1,-1),new Point(1,0),new Point(1,1),
                                        new Point(0,-1),new Point(0,1)};

         bool setVal = false;
         for (int x = 0; x < src.Width; x++)
         {
            for (int y = 0; y < src.Height; y++)
            {
               float min = float.MaxValue;

               for (int i = 0; i < neighOffset.Length; i++)
               {
                  Point off = neighOffset[i];
                  if (x + off.X < 0 || x + off.X >= src.Width ||
                     y + off.Y < 0 || y + off.Y >= src.Height)
                  {
                     continue;
                  }

                  if (src[x + off.X, y + off.Y] < min)
                     min = src[x + off.X, y + off.Y];
               }

               if (min < src[x, y])
               {
                  distMask[x, y] = valToSet;
                  setVal = true;
               }
                  
               
               dst[x, y] = min;
            }
         }
         return setVal;
      }

   }
}