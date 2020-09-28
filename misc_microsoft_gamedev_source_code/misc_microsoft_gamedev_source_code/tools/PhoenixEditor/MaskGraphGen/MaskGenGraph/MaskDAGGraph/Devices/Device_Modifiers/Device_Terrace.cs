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
   public class Device_Terrace : MaskDevice
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

      enum eMethod
      {
         eSimple = 0,
         eSharp,
         eSmooth,

         eCount
      }
      IntParam mMethod = new IntParam(0, 0, (int)eMethod.eCount);
      [ConnectionType("Param", "Method")]
      public int Method
      {
         get { return mMethod.Value; }
         set { mMethod.Value = value; }
      }

      IntParam mNumTerraces = new IntParam(1, 1, 64);
      [ConnectionType("Param", "Number of Terraces")]
      public int NumTerraces
      {
         get { return mNumTerraces.Value; }
         set { mNumTerraces.Value = value; }
      }

      FloatParam mTerraceShape = new FloatParam(0, 0.01f, 0.99f);
      [ConnectionType("Param", "Shape")]
      public float TerraceShape
      {
         get { return mTerraceShape.Value; }
         set { mTerraceShape.Value = value; }
      }

      public Device_Terrace()
      {}
      public Device_Terrace(GraphCanvas owningCanvas)
         :
          base(owningCanvas)
      {
         base.Text = "Terrace";
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
         Device_Terrace dc = fromNode as Device_Terrace;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         TerraceShape = dc.TerraceShape;
         NumTerraces = dc.NumTerraces;
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
         mp.Value = InputMask.Clone();
         mp.Value.mConstraintMask = ConstraintMask;

         float modStep = 1.0f / NumTerraces;

         if (Method == (int)eMethod.eSimple)
         {
            for (int x = 0; x < parms.Width; x++)
            {
               for (int y = 0; y < parms.Height; y++)
               {
                  float dc = InputMask[x, y] % modStep;
                  mp.Value[x, y] = InputMask[x, y]-dc;
               }
            }
         }
         else if (Method == (int)eMethod.eSharp)
         {

            for (int y = 0; y < parms.Height; y++)
            {
               
               for (int x = 0; x < parms.Width; x++)
               {
                  //find our low step
                  float val = InputMask[x, y];
                  float low = val-(val % modStep);
                  float high = low + modStep;

                  float posalpha = (val - low) / (high - low);
                  posalpha *= posalpha;
                  float posVal = ((1.0f - posalpha) * low) + (posalpha * high);

                  float negalpha = (val - low) / (high - low);
                  negalpha = 1.0f - negalpha;
                     {
                        float v = low;
                        low = high;
                        high = v;
                     }
                  negalpha *= negalpha;
                  float negVal = ((1.0f - negalpha) * low) + (negalpha * high);

                     //lerp between neg and pos values
                  mp.Value[x, y] = ((1.0f - TerraceShape) * negVal) + (TerraceShape * posVal);
               }
            }
         }
         //else if (Method == (int)eMethod.eSmooth)
         //{
         //}
         

         return true;
      }

   }
}