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
   public class Device_GaussianBlur : MaskDevice
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

      IntParam mRadius = new IntParam(1,1,30);
      [ConnectionType("Param", "Radius")]
      public int Radius
      {
         get { return mRadius.Value; }
         set { mRadius.Value = value; }
      }

      

      public Device_GaussianBlur()
      { }
      public Device_GaussianBlur(GraphCanvas owningCanvas)
         :
          base(owningCanvas)
      {
         base.Text = "Gaussian Blur";
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
         Device_GaussianBlur dc = fromNode as Device_GaussianBlur;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         Radius = dc.Radius;

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

        

         int r = Radius;
         int[] w = CreateGaussianBlurRow(r);
         int wlen = w.Length;
         DAGMask tmpMask = new DAGMask(parms.Width, parms.Height);

         
         {

            float[] waSums = new float[wlen];
            float[] wcSums = new float[wlen];
            float[] aSums = new float[wlen];
            

            for (int y = 0; y < parms.Height; ++y)
            {

               float waSum = 0;
               float wcSum = 0;
               float aSum = 0;


               int dstx = 0;

               for (int wx = 0; wx < wlen; ++wx)
               {
                  int srcX = wx - r;
                  waSums[wx] = 0;
                  wcSums[wx] = 0;
                  aSums[wx] = 0;


                  if (srcX >= 0 && srcX < parms.Width)
                  {
                     for (int wy = 0; wy < wlen; ++wy)
                     {
                        int srcY = y + wy - r;

                        if (srcY >= 0 && srcY < parms.Height)
                        {
                           float c = InputMask[srcX, srcY];
                           float wp = w[wy];

                           waSums[wx] += wp;
                           wp *= c;// +(c >> 7);
                           wcSums[wx] += wp;
                           //wp >>= 8;

                           aSums[wx] += wp * c;

                        }
                     }

                     int wwx = w[wx];
                     waSum += wwx * waSums[wx];
                     wcSum += wwx * wcSums[wx];
                     aSum += wwx * aSums[wx];

                  }
               }

              // wcSum >>= 8;

               if (waSum == 0 || wcSum == 0)
               {
                  tmpMask[dstx, y] = 0;
               }
               else
               {
                  tmpMask[dstx, y] = aSum / waSum;
               }

               ++dstx;

               for (int x = 1; x < parms.Width; ++x)
               {
                  for (int i = 0; i < wlen - 1; ++i)
                  {
                     waSums[i] = waSums[i + 1];
                     wcSums[i] = wcSums[i + 1];
                     aSums[i] = aSums[i + 1];

                  }

                  waSum = 0;
                  wcSum = 0;
                  aSum = 0;


                  int wx;
                  for (wx = 0; wx < wlen - 1; ++wx)
                  {
                     float wwx = w[wx];
                     waSum += wwx * waSums[wx];
                     wcSum += wwx * wcSums[wx];
                     aSum += wwx * aSums[wx];

                  }

                  wx = wlen - 1;

                  waSums[wx] = 0;
                  wcSums[wx] = 0;
                  aSums[wx] = 0;


                  int srcX = x + wx - r;

                  if (srcX >= 0 && srcX < parms.Width)
                  {
                     for (int wy = 0; wy < wlen; ++wy)
                     {
                        int srcY = y + wy - r;

                        if (srcY >= 0 && srcY < parms.Height)
                        {
                           float c = InputMask[srcX, srcY];
                           float wp = w[wy];

                           waSums[wx] += wp;
                           wp *= c;// +(c >> 7);
                           wcSums[wx] += wp;
                           //wp >>= 8;

                           aSums[wx] += wp * c;

                        }
                     }

                     int wr = w[wx];
                     waSum += wr * waSums[wx];
                     wcSum += wr * wcSums[wx];
                     aSum += wr * aSums[wx];

                  }

                 // wcSum >>= 8;

                  if (waSum == 0 || wcSum == 0)
                  {
                     tmpMask[x, y] = 0;
                  }
                  else
                  {
                     tmpMask[x, y] = (aSum / waSum);

                  }

                  ++dstx;
               }
            }
         }
        


         //copy back
         for (int x = 0; x < parms.Width; x++)
         {
            for (int y = 0; y < parms.Height; y++)
            {
               mp.Value[x, y] = tmpMask[x, y];
            }
         }

         tmpMask = null;

         return true;
      }

      public static int[] CreateGaussianBlurRow(int amount)
      {
         int size = 1 + (amount * 2);
         int[] weights = new int[size];

         for (int i = 0; i <= amount; ++i)
         {
            // 1 + aa - aa + 2ai - ii
            weights[i] = 16 * (i + 1);
            weights[weights.Length - i - 1] = weights[i];
         }

         return weights;
      }

   }
}