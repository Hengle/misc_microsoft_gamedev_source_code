using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.Text;
using System.Windows.Forms;
using EditorCore;
using System.Xml;
using System.Xml.Serialization;


namespace graphapp
{
   public class Device_VoronoiNoise : MaskDevice
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


       IntParam mSeedParam = new IntParam(36742,int.MinValue,int.MaxValue);
       [ConnectionType("Param", "SeedParam")]
       public int SeedParam
       {
           get { return mSeedParam.Value; }
           set { mSeedParam.Value = value; }
       }

      FloatParam mFrequency = new FloatParam(0.044f, 0, 0.172f);
       [ConnectionType("Param", "Frequency")]
       public float Frequency
       {
           get { return mFrequency.Value; }
           set { mFrequency.Value = value; }
       }

      FloatParam mDisplacement = new FloatParam(0, 0, 1.0f);
       [ConnectionType("Param", "Displacement")]
       public float Displacement
       {
           get { return mDisplacement.Value; }
           set { mDisplacement.Value = value; }
       }

       BoolParam mEnableDistance = new BoolParam(true);
       [ConnectionType("Param", "EnableDistance")]
       public bool EnableDistance
       {
           get { return mEnableDistance.Value; }
           set { mEnableDistance.Value = value; }
       }
       public Device_VoronoiNoise()
      {}
      public Device_VoronoiNoise(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {

         base.Text = "Voronoi Noise";
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
          Device_VoronoiNoise dc = fromNode as Device_VoronoiNoise;
          mGUID = dc.mGUID;
          draggedByMouse(Location, dc.Location);

          Frequency = dc.Frequency;
          SeedParam = dc.SeedParam;
          Displacement = dc.Displacement;
          EnableDistance = dc.EnableDistance;


          return true;
       }


      override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
          if (!verifyInputConnections())
              return false;

          gatherInputAndParameters(parms);

          NoiseGeneration.Voronoi pl = new NoiseGeneration.Voronoi();
          pl.mSeed = this.SeedParam;
          pl.mFrequency.Value = this.Frequency;
          pl.mDisplacement.Value = this.Displacement;
          pl.mEnableDistance = this.EnableDistance;

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