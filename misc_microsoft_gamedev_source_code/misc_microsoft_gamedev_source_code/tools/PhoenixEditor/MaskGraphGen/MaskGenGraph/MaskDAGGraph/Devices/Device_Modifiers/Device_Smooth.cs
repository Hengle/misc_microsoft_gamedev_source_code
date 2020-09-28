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

using EditorCore;
using System.Xml;
using System.Xml.Serialization;

namespace graphapp
{
   public class Device_Smooth : MaskDevice
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


     
      IntParam mSmoothPower = new IntParam(1, 1, 8);
      [ConnectionType("Param", "SmoothPower")]
      public int SmoothPower
      {
         get { return mSmoothPower.Value; }
         set { mSmoothPower.Value = value; }
      }
      public Device_Smooth()
      {}
      public Device_Smooth(GraphCanvas owningCanvas)
         :
          base(owningCanvas)
      {
         base.Text = "Smooth";
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
         Device_Smooth dc = fromNode as Device_Smooth;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         SmoothPower = dc.SmoothPower;
  
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


         ConvMatrix filter = new ConvMatrix(1, 2, 1, 2, 4, 2, 1, 2, 1);
         filter.mFactor = 16;
         filter.mOffset = 0;

         for(int i=0;i<SmoothPower;i++)
         {
            int[] neightbors = new int[] {   -1, 1,          //top left
                                           0, 1,          //top center
                                           1, 1,          //top right

                                          -1, 0,          //mid left
                                           0, 0,          //mid center
                                           1, 0,          //mid right

                                          -1, -1,          //bot left
                                           0, -1,          //bot center
                                           1, -1,          //bot right
                                          };


            DAGMask tempImgArray = new DAGMask(parms.Width, parms.Height);

            for (int x = 0; x < parms.Width; x++)
            {
               for (int y = 0; y < parms.Height; y++)
               {
                  float total = 0;
                  for (int k = 0; k < 9; k++)
                  {
                     int xIndex = x + neightbors[k * 2];
                     int zIndex = y + neightbors[k * 2 + 1];
                     if (xIndex < 0 || xIndex > parms.Width - 1 || zIndex < 0 || zIndex > parms.Height - 1)
                        continue;


                     total += filter.mFilterCoeffs[k] * mp.Value[xIndex, zIndex];

                  }
                  total = total / filter.mFactor + filter.mOffset;

                  if (total > 1.0f)
                     total = 1.0f;
                  if (total < 0)
                     total = 0;

                  tempImgArray[x, y] = total;
               }
            }

            //send our mask back
            for (int x = 0; x < parms.Width; x++)
            {
               for (int y = 0; y < parms.Height; y++)
               {
                  mp.Value[x, y] = BMathLib.Clamp(tempImgArray[x, y], 0, 1);
               }
            }


            tempImgArray = null;
         }
         

         return true;
      }


      #region convolution matrix
      public class ConvMatrix
      {
         public enum eCoeffSpot
         {
            cTopLeft = 0,
            cTopCenter,
            cTopRight,
            cMidLeft,
            cMidCenter,
            cMidRight,
            cBotLeft,
            cBotCenter,
            cBotRight

         };
         public ConvMatrix()
         {
         }
         public ConvMatrix(float topLeft, float topCenter, float topRight, float midLeft, float midCenter, float midRight, float botLeft, float botCenter, float botRight)
         {
            mFilterCoeffs = new float[9];
            mFilterCoeffs[0] = topLeft;
            mFilterCoeffs[1] = topCenter;
            mFilterCoeffs[2] = topRight;
            mFilterCoeffs[3] = midLeft;
            mFilterCoeffs[4] = midCenter;
            mFilterCoeffs[5] = midRight;
            mFilterCoeffs[6] = botLeft;
            mFilterCoeffs[7] = botCenter;
            mFilterCoeffs[8] = botRight;

         }
         public float[] mFilterCoeffs = null;
         public float mFactor = 1;
         public float mOffset = 0;
         public void SetAll(float nVal)
         {
            if (mFilterCoeffs == null)
               mFilterCoeffs = new float[9];

            for (int i = 0; i < 9; i++)
               mFilterCoeffs[i] = nVal;
         }
      }

      void tapNeightbor(int k, ref int x, ref int z, int mWidth, int mHeight)
      {
         int[] neightbors = new int[16] { +1, +0, 
                                          -1, +0,
                                          +1, +1,
                                          -1, +1,
                                          +1, -1,
                                          -1, -1,
                                          +0, +1,
                                          +0, -1 };

         x = x + neightbors[k * 2];
         z = z + neightbors[k * 2 + 1];
         BMathLib.Clamp(x, 0, mWidth - 1);
         BMathLib.Clamp(z, 0, mHeight - 1);
      }
      #endregion


   }
}

