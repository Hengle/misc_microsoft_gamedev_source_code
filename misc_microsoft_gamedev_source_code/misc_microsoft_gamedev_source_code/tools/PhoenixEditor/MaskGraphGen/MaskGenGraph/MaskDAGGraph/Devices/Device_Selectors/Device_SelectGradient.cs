using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Text;
using System.Windows.Forms;

using EditorCore;
using Terrain;
using System.Xml;
using System.Xml.Serialization;

namespace graphapp
{
    public class Device_SelectGradient : MaskDAGGraphNode
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


       public Device_SelectGradient()
      {}
        public Device_SelectGradient(GraphCanvas owningCanvas)
            :
            base(owningCanvas)
        {

            base.Text = "Select Gradient";
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
          Device_SelectGradient dc = fromNode as Device_SelectGradient;
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

            for (int x = 0; x < parms.Width; x++)
            {
                for (int y = 0; y < parms.Height; y++)
                {
                   Vector2 v = TerrainGlobals.getTerrain().getGradiant(x, y);
                   float factor = BMathLib.Clamp(v.Length(), 0, 1);
                   if (factor < 0)
                      continue;

                   mp.Value[x, y] = factor;
                }
            }

            return true;
        }
    }
}

 
                  