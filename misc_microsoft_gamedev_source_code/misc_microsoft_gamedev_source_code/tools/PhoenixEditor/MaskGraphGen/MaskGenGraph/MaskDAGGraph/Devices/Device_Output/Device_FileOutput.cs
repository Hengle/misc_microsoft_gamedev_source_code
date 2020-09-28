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
    public class Device_FileOutput : MaskDevice
   {
       MaskParam mInputMask = new MaskParam();
       [XmlIgnore]
       [ConnectionType("Input", "Primary Input", true)]
       public DAGMask InputMask
       {
           get { return mInputMask.Value; }
           set { mInputMask.Value = value; }
       }

       public Device_FileOutput()
      {}
      public Device_FileOutput(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {

         base.Text = "File Output";
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

      override public void displayPropertiesDlg()
      {
          Device_FileOutputDlg dlg = new Device_FileOutputDlg();
          dlg.mOwningDevice = this;
          dlg.ShowDialog();
      }

      public bool writeToFile(string filename, OutputGenerationParams parms)
      {
          if (!verifyInputConnections())
              return false;

          DAGMask m = mInputMask.Value;
          //temp haxor
          string ext = Path.GetExtension(filename);
          if (ext == ".r16")
          {
              FileStream s = File.Open(filename, FileMode.OpenOrCreate, FileAccess.Write);
              BinaryWriter f = new BinaryWriter(s);

              for (int x = 0; x < parms.Width; x++)
              {
                  for (int y = 0; y < parms.Height; y++)
                  {
                      
                      float k = m[x,y];
                      float scaledk = (k + 1) * 0.5f;
                      ushort v = (ushort)(scaledk * ushort.MaxValue);
                      f.Write(v);
                  }
              }
                f.Close();
                s.Close();

          }
          else if (ext == ".raw")
          {
              FileStream s = File.Open(filename, FileMode.OpenOrCreate, FileAccess.Write);
              BinaryWriter f = new BinaryWriter(s);

              for (int x = 0; x < parms.Width; x++)
              {
                  for (int y = 0; y < parms.Height; y++)
                  {

                      float k = m[x, y];
                      float scaledk = (k + 1) * 0.5f;
                      byte v = (byte)(scaledk * byte.MaxValue);
                      f.Write(v);
                  }
              }
              f.Close();
              s.Close();

          }

          return true;
      }

      override public string getDeviceDescriptor()
      {
         return "FileOutput : Double click on this node to write the mask to disk";
      }
      override public string getDeviceHelp()
      {
         return "FileOutput : This description has not yet been implimented";
      }
   }
}