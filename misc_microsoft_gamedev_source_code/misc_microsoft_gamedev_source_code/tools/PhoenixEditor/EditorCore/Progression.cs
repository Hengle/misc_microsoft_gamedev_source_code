//============================================================================
// Progression.cs
//
// Ensemble Studios, (C) 2006
//
//============================================================================

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Xml;
using System.Xml.Serialization;

namespace EditorCore
{
   [Browsable(false)]
   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public class FloatProgressionStage
   {
      private double mValue;
      private double mAlpha;
      private double mValueVariance;

      public FloatProgressionStage clone()
      {
         FloatProgressionStage clone = new FloatProgressionStage();
         clone.Value = Value;
         clone.Alpha = Alpha;
         clone.ValueVariance = ValueVariance;
         return clone;
      }

      public void preSerialization()
      {
         Alpha /= 100.0f;
         Value /= 100.0f;
         ValueVariance /= 100.0f;
      }

      public void postDeserialization()
      {
         mAlpha *= 100.0f;
         mValue *= 100.0f;
         mValueVariance *= 100.0f;
      }
      
      public double Value
      {
         get { return this.mValue; }
         set { this.mValue = value; }
      }

      public double Alpha
      {
         get { return this.mAlpha; }
         set { this.mAlpha = value; }
      }

      public double ValueVariance
      {
         get { return this.mValueVariance; }
         set { this.mValueVariance = value; }
      }

      public override string ToString()
      {
         String strValue = String.Format("({0:F2},{1:F2}) +/- [{2:F2}]", Value, Alpha, ValueVariance);
         return strValue;
      }
   }

   [Browsable(false)]
   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public class FloatProgression 
   {
      [System.Xml.Serialization.XmlArrayItemAttribute("Stage", IsNullable = false)]
      private List<FloatProgressionStage> mStages;
      private double mCycles;
      private bool   mLoop;

      public FloatProgression()
      {
         Loop = false;
         Cycles = 1.0f;
         mStages = new List<FloatProgressionStage>();
      }

      public FloatProgression clone()
      {
         FloatProgression clone = new FloatProgression();
         clone.Copy(this);
         return clone;
      }

      public void preSerialization()
      {
         for (int i = 0; i < Stages.Count; ++i)
            Stages[i].preSerialization();
      }

      public void postDeserialization()
      {
         for (int i = 0; i < Stages.Count; ++i)
            Stages[i].postDeserialization();
      }

      public void Copy(FloatProgression f)
      {
         Stages.Clear();
         for (int i = 0; i < f.Stages.Count; ++i)
         {
            addStage(f.Stages[i].Value, f.Stages[i].ValueVariance, f.Stages[i].Alpha);
         }

         Cycles = f.Cycles;
         Loop = f.Loop;
      }

      public void Scale(float factor)
      {
         for (int i = 0; i < Stages.Count; ++i)
         {
            Stages[i].Alpha *= factor;
            Stages[i].Value *= factor;
            Stages[i].ValueVariance *= factor;
         }
      }

      public bool IsEqual(FloatProgression f)
      {
         if ((Cycles != f.Cycles) || (Loop != f.Loop) || (Stages.Count != f.Stages.Count))
            return false;

         for (int i = 0; i < Stages.Count; ++i)
         {
            if ((Stages[i].Alpha != f.Stages[i].Alpha) ||
               (Stages[i].Value != f.Stages[i].Value) ||
               (Stages[i].ValueVariance != f.Stages[i].ValueVariance))
            {
               return false;
            }
         }

         return true;
      }

      [Browsable(false)]
      public List<FloatProgressionStage> Stages
      {
         get { return this.mStages; }
         set { this.mStages = value; }
      }

      [Browsable(false)]
      public double Cycles
      {
         get { return this.mCycles; }
         set { this.mCycles = value; }
      }

      [Browsable(false)]
      public bool Loop
      {
         get { return mLoop;  }
         set { mLoop = value; }
      }

      //-- interface
      public void addStage(double value, double valueVariance, double alpha)
      {
         FloatProgressionStage newStage = new FloatProgressionStage();
         newStage.Value = value;
         newStage.Alpha = alpha;
         newStage.ValueVariance = valueVariance;

         mStages.Add(newStage);
      }

      public void insertStage(double value, double valueVariance, double alpha)
      {
         int index = Stages.Count - 1;
         if (index < 0)
            index = 0;

         insertStage(index, value, valueVariance, alpha);
      }

      public void insertStage(int atIndex, double value, double valueVariance, double alpha)
      {
         FloatProgressionStage newStage = new FloatProgressionStage();
         newStage.Value = value;
         newStage.ValueVariance = valueVariance;
         newStage.Alpha = alpha;

         Stages.Insert(atIndex, newStage);
      }

      public void deleteStage(int index)
      {
         //-- cannot delete the endpoints
         if (index <= 0 || index >= mStages.Count - 1)
            return;

         mStages.RemoveAt(index);
      }

      public void moveStageUp(int index)
      {
         //-- cannot move the endpoints
         if (index <= 0 || index >= mStages.Count - 1)
            return;

         int newIndex = index - 1;
         if (newIndex < 0)
            newIndex = 0;

         //swap members
         FloatProgressionStage tempStage = mStages[newIndex];
         mStages[newIndex] = mStages[index];
         mStages[index] = tempStage;
      }

      public void moveStageDown(int index)
      {
         //-- cannot move the endpoints
         if (index < 0 || index >= mStages.Count)
            return;

         int newIndex = index + 1;
         if (newIndex >= mStages.Count)
            newIndex = mStages.Count - 1;

         //swap members
         FloatProgressionStage tempStage = mStages[newIndex];
         mStages[newIndex] = mStages[index];
         mStages[index] = tempStage;
      }
   }

   [Browsable(false)]
   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public class VectorProgression
   {
      private FloatProgression mXProgression;
      private FloatProgression mYProgression;
      private FloatProgression mZProgression;      
      public VectorProgression()
      {
         mXProgression = new FloatProgression();
         mYProgression = new FloatProgression();
         mZProgression = new FloatProgression();
      }

      public VectorProgression clone()
      {
         VectorProgression clone = new VectorProgression();

         clone.XProgression = XProgression.clone();
         clone.YProgression = YProgression.clone();
         clone.ZProgression = ZProgression.clone();

         return clone;
      }

      public void preSerialization()
      {
         XProgression.preSerialization();
         YProgression.preSerialization();
         ZProgression.preSerialization();
      }

      public void postDeserialization()
      {
         XProgression.postDeserialization();
         YProgression.postDeserialization();
         ZProgression.postDeserialization();
      }

      public FloatProgression XProgression
      {
         get { return mXProgression;  }
         set { mXProgression = value; }
      }

      public FloatProgression YProgression
      {
         get { return mYProgression; }
         set { mYProgression = value; }
      }

      public FloatProgression ZProgression
      {
         get { return mZProgression; }
         set { mZProgression = value; }
      }

   }

   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public class ColorProgression
   {
      [System.Xml.Serialization.XmlArrayItemAttribute("Stage", IsNullable = false)]
      private List<GradientPoint> mStages = new List<GradientPoint>();      
      private double mCycles;
      private bool   mLoop;

      public ColorProgression()
      {         
         mCycles = 0;
         mLoop = false;
      }

      public ColorProgression clone()
      {
         ColorProgression clone = new ColorProgression();

         clone.Loop = Loop;
         clone.Cycles = Cycles;
         clone.Stages.Clear();
         for (int i = 0; i < Stages.Count; ++i )
         {
            clone.mStages.Add(Stages[i].clone());
         }

         return clone;
      }


      public void preSerialization()
      {
         for (int i = 0; i < Stages.Count; ++i)
            Stages[i].preSerialization();
      }

      public void postDeserialization()
      {
         for (int i = 0; i < Stages.Count; ++i)
            Stages[i].postDeserialization();
      }

      public bool Loop
      {
         get { return mLoop;  }
         set { mLoop = value; }
      }

      public List<GradientPoint> Stages
      {
         get { return this.mStages; }
         set { this.mStages = value; }
      }
      
      public double Cycles
      {
         get { return this.mCycles; }
         set { this.mCycles = value; }
      }

      //-- interface
      public void addStage(Color color, double valueVariance, double alpha)
      {
         GradientPoint newStage = new GradientPoint();
         newStage.Color = color;
         newStage.Alpha = alpha;
         mStages.Add(newStage);
      }

      public void insertStage(Color color, double alpha)
      {
         int index = Stages.Count - 1;
         if (index < 0)
            index = 0;

         insertStage(index, color, alpha);
      }

      public void insertStage(int atIndex, Color color, double alpha)
      {
         GradientPoint newStage = new GradientPoint();
         newStage.Color = color;
         newStage.Alpha = alpha;

         Stages.Insert(atIndex, newStage);
      }

      public void deleteStage(int index)
      {
         //-- cannot delete the endpoints
         if (index <= 0 || index >= mStages.Count - 1)
            return;

         mStages.RemoveAt(index);
      }

      public void moveStageUp(int index)
      {
         //-- cannot move the endpoints
         if (index <= 0 || index >= mStages.Count - 1)
            return;

         int newIndex = index - 1;
         if (newIndex < 0)
            newIndex = 0;

         //swap members
         GradientPoint tempStage = mStages[newIndex];
         mStages[newIndex].Color = mStages[index].Color;
         mStages[index].Color = tempStage.Color;
      }

      public void moveStageDown(int index)
      {
         //-- cannot move the endpoints
         if (index < 0 || index >= mStages.Count)
            return;

         int newIndex = index + 1;
         if (newIndex >= mStages.Count)
            newIndex = mStages.Count - 1;

         //swap members
         GradientPoint tempStage = mStages[newIndex];
         mStages[newIndex].Color = mStages[index].Color;
         mStages[index].Color = tempStage.Color;
      }
   }

   [Browsable(false)]
   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public class GradientPoint
   {
      public GradientPoint()
      {
         mAlpha = 0.0f;
      }

      public GradientPoint clone()
      {
         GradientPoint clone = new GradientPoint();
         clone.Alpha = Alpha;
         clone.Color = Color;
         return clone;
      }

      //-- this is not transparency but a value between 0 and 1 to deterimine where the point resides in the control
      public void preSerialization()
      {
         Alpha /= 100.0f;
      }

      public void postDeserialization()
      {
         Alpha *= 100.0f;
      }

      private double mAlpha;      
      private Color mColor;

      [XmlIgnore]
      public Color Color
      {
         get { return mColor; }
         set { mColor = value; }
      }

      [XmlElement("Color")]
      public int ColorSerializer
      {
         get { return mColor.ToArgb(); }
         set { mColor = Color.FromArgb(value); }
      }

      public double Alpha
      {
         set { this.mAlpha = value; }
         get { return this.mAlpha; }
      }      

      public override string ToString()
      {
         String str = String.Format("Life (%)={0:F2} {1}", Alpha, Color);
         return str;
      }
   }
}