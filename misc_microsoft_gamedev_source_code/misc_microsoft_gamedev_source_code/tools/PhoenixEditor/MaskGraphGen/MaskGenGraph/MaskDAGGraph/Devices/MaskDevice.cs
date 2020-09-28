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

namespace graphapp
{
   public class MaskDevice : MaskDAGGraphNode
   {
       public MaskDevice()
      {}
      public MaskDevice(GraphCanvas owningCanvas)
            :
            base(owningCanvas)
        {
         }
      virtual public string getDeviceDescriptor()
      {
         return "Device : This description has not yet been implimented";
      }
      virtual public string getDeviceHelp()
      {
         return "Device : This description has not yet been implimented";
      }
   }
}