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
   public class Device_Inverter : MaskDevice
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

       public Device_Inverter()
      {}
        public Device_Inverter(GraphCanvas owningCanvas)
            :
            base(owningCanvas)
        {
            base.Text = "Inverter";
            mColorTop = Color.White;
            mColorBottom = Color.CornflowerBlue;
            mBorderSize = 1;

            mSize.Width = 60;
            mSize.Height = 20;

            generateConnectionPoints();
            resizeFromConnections();
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

            for (int x = 0; x < parms.Width; x++)
            {
                for (int y = 0; y < parms.Height; y++)
                {
                    mp.Value[x, y] = (1.0f-mp.Value[x, y]);
                }
            }

            return true;
        }

    }
}