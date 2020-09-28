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
   public class Device_SelectSlope : MaskDevice
    {
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



        FloatParam mMinSlope = new FloatParam(0.0f, 0, 1);
        [ConnectionType("Param", "MinSlope")]
        public float MinSlope
        {
            get { return mMinSlope.Value; }
            set { mMinSlope.Value = value; }
        }
        FloatParam mMaxSlope = new FloatParam(0.5f, 0, 1);
        [ConnectionType("Param", "MaxSlope")]
        public float MaxSlope
        {
            get { return mMaxSlope.Value; }
            set { mMaxSlope.Value = value; }
        }
        FloatParam mFalloffAmt = new FloatParam(0.25f, 0, 1);
        [ConnectionType("Param", "FalloffAmt")]
        public float FalloffAmt
        {
            get { return mFalloffAmt.Value; }
            set { mFalloffAmt.Value = value; }
        }


       public Device_SelectSlope()
      {}
        public Device_SelectSlope(GraphCanvas owningCanvas)
            :
            base(owningCanvas)
        {

            base.Text = "Select Slope";
            mColorTop = Color.White;
            mColorBottom = Color.Purple;
            mBorderSize = 1;

            mSize.Width = 60;
            mSize.Height = 20;

            generateConnectionPoints();
            resizeFromConnections();
        }
       public override bool load(MaskDAGGraphNode fromNode)
       {
          Device_SelectSlope dc = fromNode as Device_SelectSlope;
          mGUID = dc.mGUID;
          draggedByMouse(Location, dc.Location);

          FalloffAmt = dc.FalloffAmt;
          MaxSlope = dc.MaxSlope;
          MinSlope = dc.MinSlope;

          return true;
       }

    
        override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
        {
            if (!verifyInputConnections())
                return false;

             if (!gatherInputAndParameters(parms))
                return false;
            MaskParam mp = ((MaskParam)(connPoint.ParamType));
            mp.Value = new DAGMask(InputMask.Width, InputMask.Height);

            float tileScale = 1.0f;
            

            for (int x = 0; x < parms.Width; x++)
            {
                for (int y = 0; y < parms.Height; y++)
                {
                   //CLM [02.18.08] this uses the calculation of the Gradiant for a given heightfield element
                   // That is, the partial derivitive of the element.
                   // taking the length approximates how much gradation exists in either the X or Y directions.

                   int px = (int)BMathLib.Clamp(x + 1, 0, parms.Width - 1);
                   int py = (int)BMathLib.Clamp(y + 1, 0, parms.Height - 1);

                   int nx = (int)BMathLib.Clamp(x - 1, 0, parms.Width - 1);
                   int ny = (int)BMathLib.Clamp(y - 1, 0, parms.Height - 1);

                   float vX = (InputMask[px, y] - InputMask[nx, y]) / (2 * tileScale);
                   float vY = (InputMask[x, py] - InputMask[x, ny]) / (2 * tileScale);

                   float vLen = (float)Math.Sqrt(vX * vX + vY * vY);

                   float slope = BMathLib.Saturate(vLen);



                    if (slope >= MinSlope && slope <= MaxSlope)
                    {
                        mp.Value[x, y] = 1.0f;
                    }
                    else if (slope >= MinSlope - FalloffAmt && slope <= MinSlope)
                    {
                        mp.Value[x, y] = 1 - ((MinSlope - slope) / FalloffAmt);
                    }
                    else if (slope >= MaxSlope && slope <= MaxSlope + FalloffAmt)
                    {
                        mp.Value[x, y] = 1 - ((slope - MaxSlope) / FalloffAmt);
                    }
                    else
                    {
                        mp.Value[x, y] = 0;
                    }
                }
            }

            return true;
        }
    }
}