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
   public class Device_Chooser : MaskDevice
    {
        MaskParam mInputMaskA = new MaskParam();
       [XmlIgnore]
        [ConnectionType("Input", "Primary Input A",true)]
       public DAGMask InputMaskA
        {
            get { return mInputMaskA.Value; }
            set { mInputMaskA.Value = value; }
        }

        MaskParam mInputMaskB = new MaskParam();
       [XmlIgnore]
        [ConnectionType("Input", "Primary Input B",true)]
       public DAGMask InputMaskB
        {
            get { return mInputMaskB.Value; }
            set { mInputMaskB.Value = value; }
        }

        MaskParam mOutputMask = new MaskParam();
       [XmlIgnore]
        [ConnectionType("Output", "Primary Output")]
       public DAGMask OutputMask
        {
            get { return mOutputMask.Value; }
            set { mOutputMask.Value = value; }
        }

        BoolParam mInputChoice = new BoolParam(true);
        [ConnectionType("Param", "InputChoice")]
        public bool InputChoice
        {
            get { return mInputChoice.Value; }
            set { mInputChoice.Value = value; }
        }

       public Device_Chooser()
      {}
        public Device_Chooser(GraphCanvas owningCanvas)
            :
            base(owningCanvas)
        {
            base.Text = "Chooser";
            mColorTop = Color.White;
            mColorBottom = Color.PeachPuff;
            mBorderSize = 1;

            mSize.Width = 60;
            mSize.Height = 20;

            generateConnectionPoints();
            resizeFromConnections();
        }
       public override bool load(MaskDAGGraphNode fromNode)
       {
          Device_Chooser dc = fromNode as Device_Chooser;
          mGUID = dc.mGUID;
          draggedByMouse(Location, dc.Location);

          InputChoice = dc.InputChoice;
        
          return true;
       }
        override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
        {
            if (!verifyInputConnections())
                return false;

             if (!gatherInputAndParameters(parms))
                return false;


            MaskParam mp = ((MaskParam)(connPoint.ParamType));

            if (InputChoice)
            {
                mp.Value = InputMaskA.Clone();
            }
            else
            {
                mp.Value = InputMaskB.Clone();
            }

            return true;
        }

    }
}