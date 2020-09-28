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
   public class Device_MedianBlur : MaskDevice
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

      IntParam mRadius = new IntParam(10, 1, 200);
      [ConnectionType("Param", "Radius")]
      public int Radius
      {
         get { return mRadius.Value; }
         set { mRadius.Value = value; }
      }

      IntParam mPercent = new IntParam(50, 0, 200);
      [ConnectionType("Param", "Percent")]
      public int Percent
      {
         get { return mPercent.Value; }
         set { mPercent.Value = value; }
      }

      
      public Device_MedianBlur()
      { }
      public Device_MedianBlur(GraphCanvas owningCanvas)
         :
          base(owningCanvas)
      {
         base.Text = "Median Blur";
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
         Device_MedianBlur dc = fromNode as Device_MedianBlur;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         Radius = dc.Radius;
         Percent = dc.Percent;

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



         ///////////////////////////////////////////////////////////////

         int width = parms.Width;
         int height = parms.Height;
         int rad = Radius;

         int[] leadingEdgeX = new int[rad + 1];
        
         // approximately (rad + 0.5)^2
         int cutoff = ((rad * 2 + 1) * (rad * 2 + 1) + 2) / 4;

         for (int v = 0; v <= rad; ++v)
         {
            for (int u = 0; u <= rad; ++u)
            {
               if (u * u + v * v <= cutoff)
               {
                  leadingEdgeX[v] = u;
               }
            }
         }

         const int hLength = 256;
         
         int[] ha = new int[hLength];
         
         for (int y = 0; y < parms.Height; y++)
         {

            
            
            //calculate our histogram..
            for (int k = 0; k < hLength; k++)
               ha[k] = 0;

            int area = 0;
            int maxArea = GetMaxAreaForRadius(rad);

            int top = -Math.Min(rad, y);
            int bottom = Math.Min(rad, height - 1 - y);
            int left = -Math.Min(rad, 0);
            int right = Math.Min(rad, width - 1);

            
            for (int v = top; v <= bottom; ++v)
            {
               for (int u = left; u <= right; ++u)
               {
                  byte psamp = (byte)(InputMask[u, y + v] * 255);

                  if ((u * u + v * v) <= cutoff)
                  {
                     ++area;
                   
                     ++ha[psamp];
                  }
               }
            }
            




            for (int x = 0; x < parms.Width; x++)
            {
               mp.Value[x,y] = GetPercentile(Percent, area, ref ha)/255.0f;

               left = -Math.Min(rad, x);
               right = Math.Min(rad + 1, width - 1 - x);

               // Subtract trailing edge top half
               int v = -1;

               while (v >= top)
               {
                  int u = leadingEdgeX[-v];

                  if (-u >= left)
                     break;
                 
                  --v;
               }

               while (v >= top)
               {
                  int u = leadingEdgeX[-v];
                  byte p = (byte)(InputMask[x - u, y + v] * 255);

                
                  --ha[p];
                  --area;

                  --v;
               }

               // add leading edge top half
               v = -1;
               while (v >= top)
               {
                  int u = leadingEdgeX[-v];

                  if (u + 1 <= right)
                     break;
                  
                  --v;
               }

               while (v >= top)
               {
                  int u = leadingEdgeX[-v];
                  byte p = (byte)(InputMask[x + u +1, y + v] * 255);
                  
                  ++ha[p];
                  ++area;

                  --v;
               }

               // Subtract trailing edge bottom half
               v = 0;

               while (v <= bottom)
               {
                  int u = leadingEdgeX[v];

                  if (-u >= left)
                     break;

                  ++v;
               }

               while (v <= bottom)
               {
                  int u = leadingEdgeX[v];
                  byte p = (byte)(InputMask[x - u, y + v] * 255);

                  --ha[p];
                  --area;

                  ++v;
               }

               // add leading edge bottom half
               v = 0;

               while (v <= bottom)
               {
                  int u = leadingEdgeX[v];

                  if (u + 1 <= right)
                     break;
                  

                  ++v;
               }

               while (v <= bottom)
               {
                  int u = leadingEdgeX[v];
                  byte p = (byte)(InputMask[x + u +1, y + v] * 255);
                 
                  ++ha[p];
                  ++area;

                  ++v;
               }
            }
         }

         return true;
      }

      protected static int GetMaxAreaForRadius(int radius)
      {
         int area = 0;
         int cutoff = ((radius * 2 + 1) * (radius * 2 + 1) + 2) / 4;

         for (int v = -radius; v <= radius; ++v)
         {
            for (int u = -radius; u <= radius; ++u)
            {
               if (u * u + v * v <= cutoff)
               {
                  ++area;
               }
            }
         }

         return area;
      }
      protected static int GetPercentile(int percentile, int area,ref int[] ha)
      {
         int minCount = area * percentile / 100;

       
         int a = 0;
         int aCount = 0;

         while (a < 255 && ha[a] == 0)
         {
            ++a;
         }

         while (a < 255 && aCount < minCount)
         {
            aCount += ha[a];
            ++a;
         }

         return a;
      }

   }
}