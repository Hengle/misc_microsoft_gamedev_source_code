using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.Xml;
using System.Xml.Serialization;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace graphapp
{
   public class Device_PerlinNoise : MaskDevice
   {
      MaskParam mConstraintMask = new MaskParam();
       [XmlIgnore]
      [ConnectionType("Constraint", "Primary Constraint")]
      public DAGMask ConstraintMask
      {
         get { return mConstraintMask.Value; }
         set { mConstraintMask.Value = value; }
      }

        MaskParam mOutputMask = new MaskParam();
       [XmlIgnore]
        [ConnectionType("Output", "Primary Output")]
       public DAGMask OutputMask
        {
            get { return mOutputMask.Value; }
            set { mOutputMask.Value = value; }
        }

        FloatParam mPersistance = new FloatParam(0.5f,0,1);
        [ConnectionType("Param", "Persistance")]
        public float Persistance
        {
            get { return mPersistance.Value; }
            set { mPersistance.Value = value; }
        }

       FloatParam mFrequency = new FloatParam(0.044f, 0, 0.172f);
        [ConnectionType("Param", "Frequency")]
        public float Frequency
        {
            get { return mFrequency.Value; }
            set { mFrequency.Value = value; }
        }
       FloatParam mLacunarity = new FloatParam(2.0f, 1.5f, 3.5f);
        [ConnectionType("Param", "Lacunarity")]
        public float Lacunarity
        {
            get { return mLacunarity.Value; }
            set { mLacunarity.Value = value; }
        }

        IntParam mOctaveParam = new IntParam(6,0,30);
        [ConnectionType("Param", "OctaveParam")]
        public int OctaveParam
        {
            get { return mOctaveParam.Value; }
            set { mOctaveParam.Value = value; }
        }

        IntParam mSeedParam = new IntParam(36742,int.MinValue,int.MaxValue);
        [ConnectionType("Param", "SeedParam")]
        public int SeedParam
        {
            get { return mSeedParam.Value; }
            set { mSeedParam.Value = value; }
        }

       public Device_PerlinNoise()
       {
       }

      public Device_PerlinNoise(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {

         base.Text = "Perlin Noise";
         mColorTop = Color.White;
         mColorBottom = Color.Green;
         mBorderSize = 1;

         mSize.Width = 60;
         mSize.Height = 20;



         generateConnectionPoints();
         resizeFromConnections();
      }

       public override bool load(MaskDAGGraphNode fromNode)
       {
          Device_PerlinNoise dc = fromNode as Device_PerlinNoise;
          mGUID = dc.mGUID;
          draggedByMouse(Location, dc.Location);

          Persistance = dc.Persistance;
          Frequency = dc.Frequency;
          Lacunarity = dc.Lacunarity;
          OctaveParam = dc.OctaveParam;
          SeedParam = dc.SeedParam;

          
          return true;
       }

      override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
          if (!verifyInputConnections())
              return false;

          gatherInputAndParameters(parms);

          NoiseGeneration.Perlin pl = new NoiseGeneration.Perlin();
          pl.mSeed = this.SeedParam;
          pl.mPersistence.Value = this.Persistance;
          pl.mOctaveCount.Value = this.OctaveParam;
          pl.mFrequency.Value = this.Frequency;
          pl.mLacunarity.Value = this.Lacunarity;

          int sWidth = 128;
          int sHeight = 128;

          MaskParam mp = ((MaskParam)(connPoint.ParamType));
          mp.Value = new DAGMask(parms.Width, parms.Height);
          mp.Value.mConstraintMask = ConstraintMask;

          for (int y = 0; y < parms.Height; y++)
          {
             for (int x = 0; x < parms.Width; x++)
             {
                mp.Value[x, y] = BMathLib.Clamp((float)((pl.getValue((x / (float)parms.Width) * sWidth, (y / (float)parms.Height) * sHeight, 0) + 1) * 0.5f), 0, 1);
             }
          }
          return true;
      }

       
   }
}