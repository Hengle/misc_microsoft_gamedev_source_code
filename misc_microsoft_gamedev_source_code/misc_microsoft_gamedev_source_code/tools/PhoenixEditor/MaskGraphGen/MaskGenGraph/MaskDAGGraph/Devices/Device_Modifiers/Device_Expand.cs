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
   public class Device_Expand : MaskDevice
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

      IntParam mNumPixels = new IntParam(1,1,64);
      [ConnectionType("Param", "Size")]
      public int NumPixels 
      {
         get { return mNumPixels.Value; }
         set { mNumPixels.Value = value; }
      }



      public Device_Expand()
      { }
      public Device_Expand(GraphCanvas owningCanvas)
         :
          base(owningCanvas)
      {
         base.Text = "Expand";
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
         Device_Expand dc = fromNode as Device_Expand;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         NumPixels = dc.NumPixels;

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

        
         DAGMask outMask = InputMask.Clone();

         for (int i = 0; i < NumPixels; i++)
         {
            DAGMask smallerMask = new DAGMask(parms.Width, parms.Height);
            contractSetValue(ref outMask, ref smallerMask);

            outMask = smallerMask.Clone();
         }


         //normalize our input now...
         for (int x = 0; x < parms.Width; x++)
         {
            for (int y = 0; y < parms.Height; y++)
            {
               mp.Value[x, y] = outMask[x, y];
            }
         }

         outMask = null;

         return true;
      }

      void contractSetValue(ref DAGMask src, ref DAGMask dst)
      {
         Point[] neighOffset = new Point[]{new Point(-1,-1),new Point(-1,0),new Point(-1,1),
                                        new Point(1,-1),new Point(1,0),new Point(1,1),
                                        new Point(0,-1),new Point(0,1)};

         for (int x = 0; x < src.Width; x++)
         {
            for (int y = 0; y < src.Height; y++)
            {
               float max = float.MinValue;

               for (int i = 0; i < neighOffset.Length; i++)
               {
                  Point off = neighOffset[i];
                  if (x + off.X < 0 || x + off.X >= src.Width ||
                     y + off.Y < 0 || y + off.Y >= src.Height)
                  {
                     continue;
                  }

                  if (src[x + off.X, y + off.Y] > max)
                     max = src[x + off.X, y + off.Y];
               }
               dst[x, y] = max;
            }
         }
      }

   }
}