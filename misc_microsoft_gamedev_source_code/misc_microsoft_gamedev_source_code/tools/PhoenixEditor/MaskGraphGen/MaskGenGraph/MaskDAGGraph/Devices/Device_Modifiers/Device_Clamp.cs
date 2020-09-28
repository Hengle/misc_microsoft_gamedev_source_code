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
   public class Device_Clamp : MaskDevice
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
        [ConnectionType("Input", "Primary Input",true)]
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



        FloatParam mMinHeight = new FloatParam(0,0,1);
        [ConnectionType("Param", "MinHeight")]
        public float MinHeight
        {
            get { return mMinHeight.Value; }
            set { mMinHeight.Value = value; }
        }

        FloatParam mMaxHeight = new FloatParam(0.5f,0,1);
        [ConnectionType("Param", "MaxHeight")]
        public float MaxHeight
        {
            get { return mMaxHeight.Value; }
            set { mMaxHeight.Value = value; }
        }

        enum eMethod
        {
            eScale=0,
            eClamp,

            eCount
        }
        IntParam mClampType = new IntParam(0, 0, (int)eMethod.eCount);
        [ConnectionType("Param", "ClampType")]
        public int ClampType
        {
            get { return mClampType.Value; }
            set { mClampType.Value = value; }
        }
       public Device_Clamp()
      {}
        public Device_Clamp(GraphCanvas owningCanvas)
            :
            base(owningCanvas)
        {
            base.Text = "Clamp";
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
          Device_Clamp dc = fromNode as Device_Clamp;
          mGUID = dc.mGUID;
          draggedByMouse(Location, dc.Location);

          ClampType = dc.ClampType;
          MinHeight = dc.MinHeight;
          MaxHeight = dc.MaxHeight;

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

            if (ClampType == (int)eMethod.eScale) //scale between the values
            {
                float delta = MaxHeight - MinHeight;

                for (int x = 0; x < parms.Width; x++)
                {
                    for (int y = 0; y < parms.Height; y++)
                    {
                        mp.Value[x, y] = MinHeight + (InputMask[x, y] * delta);
                    }
                }
            }

            else if (ClampType == (int)eMethod.eClamp) //actually clamp
            {
                for (int x = 0; x < parms.Width; x++)
                {
                    for (int y = 0; y < parms.Height; y++)
                    {
                       mp.Value[x, y] = BMathLib.Clamp(InputMask[x, y], MinHeight, MaxHeight);
                    }
                }

            }
            
            return true;
        }

    }
}