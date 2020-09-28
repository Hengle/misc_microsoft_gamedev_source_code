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
   public class Device_Combiner : MaskDevice
    {
       MaskParam mConstraintMask = new MaskParam();
       [XmlIgnore]
       [ConnectionType("Constraint", "Primary Constraint")]
       public DAGMask ConstraintMask
       {
          get { return mConstraintMask.Value; }
          set { mConstraintMask.Value = value; }
       }

        MaskParam mInputMaskA = new MaskParam();
       [XmlIgnore]
       [ConnectionType("Input", "Primary Input A", true)]
       public DAGMask InputMaskA
        {
            get { return mInputMaskA.Value; }
            set { mInputMaskA.Value = value; }
        }

        MaskParam mInputMaskB = new MaskParam();
       [XmlIgnore]
       [ConnectionType("Input", "Primary Input B", true)]
       public DAGMask InputMaskB
        {
            get { return mInputMaskB.Value; }
            set { mInputMaskB.Value = value; }
        }

        MaskParam mStrength = new MaskParam();
       [XmlIgnore]
       [ConnectionType("Input", "Strength", true)]
       public DAGMask Strength
        {
            get { return mStrength.Value; }
            set { mStrength.Value = value; }
        }

        MaskParam mOutputMask = new MaskParam();
       [XmlIgnore]
        [ConnectionType("Output", "Primary Output")]
       public DAGMask OutputMask
        {
            get { return mOutputMask.Value; }
            set { mOutputMask.Value = value; }
        }




        enum eMethod
        {
            eAverage =0,
            eAdd,
            eSubtract,
            eMultiply,
            eMax,
            eMin,
            eLerp,


            eCount
        }
        IntParam mMethod = new IntParam(0, 0, (int)eMethod.eCount);
        [ConnectionType("Param", "Method")]
        public int Method
        {
            get { return mMethod.Value; }
            set { mMethod.Value = value; }
        }
       public Device_Combiner()
      {}
        public Device_Combiner(GraphCanvas owningCanvas)
            :
            base(owningCanvas)
        {
            base.Text = "Combiner";
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
          Device_Combiner dc = fromNode as Device_Combiner;
          mGUID = dc.mGUID;
          draggedByMouse(Location, dc.Location);

          Method = dc.Method;
        
          return true;
       }

        override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
        {
            if (!verifyInputConnections())
                return false;

             if (!gatherInputAndParameters(parms))
                return false;

            MaskParam mp = ((MaskParam)(connPoint.ParamType));
            mp.Value = new DAGMask(InputMaskA.Width, InputMaskA.Height);
            mp.Value.mConstraintMask = ConstraintMask;

            if (Method == (int)eMethod.eAverage) //AVERAGE
            {
     
                for (int x = 0; x < parms.Width; x++)
                {
                    for (int y = 0; y < parms.Height; y++)
                    {
                        mp.Value[x, y] = (InputMaskA[x, y] + InputMaskB[x, y]) * Strength[x,y];
                    }
                }
            }
            else if (Method == (int)eMethod.eAdd) //ADD
            {
 
                for (int x = 0; x < parms.Width; x++)
                {
                    for (int y = 0; y < parms.Height; y++)
                    {
                        mp.Value[x, y] = InputMaskA[x, y] + (InputMaskB[x, y] * Strength[x, y]);
                    }
                }
            }
            else if (Method == (int)eMethod.eSubtract) //SUBTRACT
            {

                for (int x = 0; x < parms.Width; x++)
                {
                    for (int y = 0; y < parms.Height; y++)
                    {
                        mp.Value[x, y] = InputMaskA[x, y] - (InputMaskB[x, y] * Strength[x, y]);
                    }
                }
            }
            else if (Method == (int)eMethod.eMultiply) //MULTIPLY
            {
        
                for (int x = 0; x < parms.Width; x++)
                {
                    for (int y = 0; y < parms.Height; y++)
                    {
                        mp.Value[x, y] = InputMaskA[x, y] * InputMaskB[x, y] * Strength[x, y];
                    }
                }
            }
            else if (Method == (int)eMethod.eMax) //MAX
            {

                for (int x = 0; x < parms.Width; x++)
                {
                    for (int y = 0; y < parms.Height; y++)
                    {
                        mp.Value[x, y] = Math.Max(InputMaskA[x, y] , InputMaskB[x, y]);
                    }
                }
            }
            else if (Method == (int)eMethod.eMin) //MIN
            {

                for (int x = 0; x < parms.Width; x++)
                {
                    for (int y = 0; y < parms.Height; y++)
                    {
                        mp.Value[x, y] = Math.Min(InputMaskA[x, y], InputMaskB[x, y]);
                    }
                }
            }
            else if (Method == (int)eMethod.eLerp) //LERP
            {

                for (int x = 0; x < parms.Width; x++)
                {
                    for (int y = 0; y < parms.Height; y++)
                    {
                       mp.Value[x, y] = ((1.0f - Strength[x, y]) * InputMaskA[x, y]) + (Strength[x, y] * InputMaskB[x, y]);
                    }
                }
            }

            return true;
        }

    }
}