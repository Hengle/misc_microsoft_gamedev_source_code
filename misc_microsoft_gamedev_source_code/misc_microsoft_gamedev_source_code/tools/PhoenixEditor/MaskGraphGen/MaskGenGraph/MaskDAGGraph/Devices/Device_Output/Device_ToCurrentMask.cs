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
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using EditorCore;


namespace graphapp
{
   public class Device_ToCurrentMask : MaskDevice
   {
      MaskParam mInputMask = new MaskParam();
      [XmlIgnore]
      [ConnectionType("Input", "Primary Input", true)]
      public DAGMask InputMask
      {
         get { return mInputMask.Value; }
         set { mInputMask.Value = value; }
      }

      public Device_ToCurrentMask()
      {}
      public Device_ToCurrentMask(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {

         base.Text = "To Current Mask";
         mColorTop = Color.White;
         mColorBottom = Color.Red;
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


         return true;
      }
      public DAGMask ResultMask
      {
         get { return InputMask; }
      }

      override public string getDeviceDescriptor()
      {
         return "ToCurrentMask : This is the final of any graph";
      }
      override public string getDeviceHelp()
      {
         return "ToCurrentMask : This description has not yet been implimented";
      }
   }
}