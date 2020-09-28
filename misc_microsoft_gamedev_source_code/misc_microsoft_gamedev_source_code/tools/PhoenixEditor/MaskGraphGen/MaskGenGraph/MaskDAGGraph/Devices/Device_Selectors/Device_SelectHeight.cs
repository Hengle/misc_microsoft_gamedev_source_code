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
   public class Device_SelectHeight : MaskDevice
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



       FloatParam mMinHeight = new FloatParam(0.0f, 0, 1);
       [ConnectionType("Param", "MinHeight")]
       public float MinHeight
       {
           get { return mMinHeight.Value; }
           set { mMinHeight.Value = value; }
       }
       FloatParam mMaxHeight = new FloatParam(0.5f, 0, 1);
       [ConnectionType("Param", "MaxHeight")]
       public float MaxHeight
       {
           get { return mMaxHeight.Value; }
           set { mMaxHeight.Value = value; }
       }
       FloatParam mFalloffAmt = new FloatParam(0.25f,0,1);
       [ConnectionType("Param", "FalloffAmt")]
       public float FalloffAmt
       {
           get { return mFalloffAmt.Value; }
           set { mFalloffAmt.Value = value; }
       }


       public Device_SelectHeight()
      {}
      public Device_SelectHeight(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {

         base.Text = "Select Height";
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
          Device_SelectHeight dc = fromNode as Device_SelectHeight;
          mGUID = dc.mGUID;
          draggedByMouse(Location, dc.Location);

          FalloffAmt = dc.FalloffAmt;
          MaxHeight = dc.MaxHeight;
          MinHeight = dc.MinHeight;

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
                  if (InputMask[x, y] >= MinHeight && InputMask[x, y] <= MaxHeight)
                  {
                      mp.Value[x, y] = 1.0f;
                  }
                  else if (InputMask[x, y] >= MinHeight - FalloffAmt && InputMask[x, y] <= MinHeight)
                  {
                      mp.Value[x, y] = 1-((MinHeight - InputMask[x, y]) / FalloffAmt);
                  }
                  else if (InputMask[x, y] >= MaxHeight && InputMask[x, y] <= MaxHeight + FalloffAmt)
                  {
                      mp.Value[x, y] = 1-((InputMask[x, y]-MaxHeight) / FalloffAmt);
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