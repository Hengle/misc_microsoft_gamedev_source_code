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
using NoiseGeneration;
namespace graphapp
{
   public class Device_Distort : MaskDevice
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

      

      FloatParam mFrequency = new FloatParam(0.025f, 0,0.2f);
      [ConnectionType("Param", "Frequency")]
      public float Frequency
      {
         get { return mFrequency.Value; }
         set { mFrequency.Value = value; }
      }
      
      FloatParam mPower = new FloatParam(0.1f, 0.02f, 0.5f);
      [ConnectionType("Param", "Power")]
      public float Power
      {
         get { return mPower.Value; }
         set { mPower.Value = value; }
      }



      IntParam mRoughness = new IntParam(3, 0, 10);
      [ConnectionType("Param", "Roughness")]
      public int Roughness
      {
         get { return mRoughness.Value; }
         set { mRoughness.Value = value; }
      }

      IntParam mRandomSeed = new IntParam(1, 0, int.MaxValue);
      [ConnectionType("Param", "RandomSeed")]
      public int RandomSeed
      {
         get { return mRandomSeed.Value; }
         set { mRandomSeed.Value = value; }
      }


      NoiseGeneration.Perlin mXDistort = new NoiseGeneration.Perlin();
      NoiseGeneration.Perlin mYDistort = new NoiseGeneration.Perlin();

      public Device_Distort()
      {}
      public Device_Distort(GraphCanvas owningCanvas)
         :
          base(owningCanvas)
      {
         base.Text = "Distort";
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
         Device_Distort dc = fromNode as Device_Distort;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         Frequency = dc.Frequency;
         Power = dc.Power;
         Roughness = dc.Roughness;
         RandomSeed = dc.RandomSeed;

         return true;
      }

      void setNoiseParms()
      {
         mXDistort.mFrequency.Value = Frequency;
         mYDistort.mFrequency.Value = Frequency;

         mXDistort.mOctaveCount.Value = Roughness;
         mYDistort.mOctaveCount.Value = Roughness;
         
         mXDistort.mSeed = RandomSeed;
         mYDistort.mSeed = RandomSeed + 1;
      }
      override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
         if (!verifyInputConnections())
            return false;

         if (!gatherInputAndParameters(parms))
            return false;

         setNoiseParms();


         MaskParam mp = ((MaskParam)(connPoint.ParamType));
         mp.Value = InputMask.Clone();
         mp.Value.mConstraintMask = ConstraintMask;


         for (int y = 0; y < parms.Height; y++)
         {
            for (int x = 0; x < parms.Width; x++)
            {
               float x0, y0;
               float x1, y1;
               float x2, y2;
               x0 = x + (12414.0f / 65536.0f);
               y0 = y + (65124.0f / 65536.0f);
               
               x1 = x + (26519.0f / 65536.0f);
               y1 = y + (18128.0f / 65536.0f);
               
               x2 = x + (53820.0f / 65536.0f);
               y2 = y + (11213.0f / 65536.0f);

               float xDistort = ((float)(mXDistort.getValue(x0, y0, 0) * Power));
               float yDistort = ((float)(mYDistort.getValue(x1, y1, 0) * Power));


               int kx = (int)BMathLib.Clamp(x + (int)(xDistort * parms.Width),0,parms.Width-1);
               int ky = (int)BMathLib.Clamp(y + (int)(yDistort * parms.Height), 0, parms.Height-1);

               mp.Value[x, y] = InputMask[kx, ky];
            }
         }

         return true;
      }

   }
}