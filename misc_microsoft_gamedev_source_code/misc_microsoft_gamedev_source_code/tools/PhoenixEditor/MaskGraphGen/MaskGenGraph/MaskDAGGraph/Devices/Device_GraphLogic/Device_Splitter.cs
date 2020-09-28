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
   public class Device_Splitter : MaskDevice
   {
        MaskParam mInputMask = new MaskParam();
       [XmlIgnore]
       [ConnectionType("Input", "Primary Input",true)]
       public DAGMask InputMask
       {
           get { return mInputMask.Value; }
           set { mInputMask.Value = value; }
       }

       MaskParam mOutputMaskA = new MaskParam();
       [XmlIgnore]
       [ConnectionType("Output", "Primary Output A")]
       public DAGMask OutputMaskA
       {
           get { return mOutputMaskA.Value; }
           set { mOutputMaskA.Value = value; }
       }

       MaskParam mOutputMaskB = new MaskParam();
       [XmlIgnore]
       [ConnectionType("Output", "Primary Output B")]
       public DAGMask OutputMaskB
       {
           get { return mOutputMaskB.Value; }
           set { mOutputMaskB.Value = value; }
       }

       MaskParam mOutputMaskC = new MaskParam();
       [XmlIgnore]
       [ConnectionType("Output", "Primary Output C")]
       public DAGMask OutputMaskC
       {
           get { return mOutputMaskC.Value; }
           set { mOutputMaskC.Value = value; }
       }

       public Device_Splitter()
      {}
      public Device_Splitter(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {
         base.Text = "Splitter";
         mColorTop = Color.White;
         mColorBottom = Color.PeachPuff;
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
          
          //this is a splitter, so we need to duplicate the input values..
          MaskParam mp = ((MaskParam)(connPoint.ParamType));
          mp.Value = InputMask.Clone();

          return true;
      }

   }
}