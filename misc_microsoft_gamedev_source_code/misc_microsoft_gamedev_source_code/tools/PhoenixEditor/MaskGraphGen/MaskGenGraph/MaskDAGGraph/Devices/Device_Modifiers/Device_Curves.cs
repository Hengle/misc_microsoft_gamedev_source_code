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
   public class Device_Curves : MaskDevice
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


      /*
      * CLM this shit sucks.
      * C# 2.0 can't seralize anything from IDictionary, which SortedList just so happens to be. 
      * So split the sorted list into 2 lists
      * We trust that our dialog will properly sort / unsort these for us..
      */
      protected List<float> mControlPointsKeys = new List<float>();
      public List<float> ControlPointsKeys
      {
         get
         {
            return mControlPointsKeys;
         }

         set
         {
            mControlPointsKeys = value;
         }
      }

      protected List<float> mControlPointsValues = new List<float>();
      public List<float> ControlPointsValues
      {
         get
         {
            return mControlPointsValues;
         }

         set
         {
            mControlPointsValues = value;
         }
      }

     

      public override bool load(MaskDAGGraphNode fromNode)
      {
         Device_Curves dc = fromNode as Device_Curves;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         ControlPointsValues.Clear();
         ControlPointsKeys.Clear();

         ControlPointsValues.AddRange(dc.ControlPointsValues);
         ControlPointsKeys.AddRange(dc.ControlPointsKeys);


         return true;
      }

      public Device_Curves()
      { }
      public Device_Curves(GraphCanvas owningCanvas)
         :
          base(owningCanvas)
      {
         base.Text = "Curves";
         mColorTop = Color.White;
         mColorBottom = Color.CornflowerBlue;
         mBorderSize = 1;

         mSize.Width = 60;
         mSize.Height = 20;

         generateConnectionPoints();
         resizeFromConnections();

         addControlPoint(0, 0);
         addControlPoint(1, 1);
         

      }

      public void addControlPoint(float key, float value)
      {
         ControlPointsKeys.Add(key);
         ControlPointsValues.Add(value);
      }
      public void clearControlPoints()
      {
         ControlPointsKeys.Clear();
         ControlPointsValues.Clear();
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

         SplineInterpolator interpolator = new SplineInterpolator();
         for (int i = 0; i < ControlPointsKeys.Count; ++i)
            interpolator.Add(ControlPointsKeys[i], ControlPointsValues[i]);
         

         for (int x = 0; x < parms.Width; x++)
         {
            for (int y = 0; y < parms.Height; y++)
            {
               float v = mp.Value[x, y];
               
               mp.Value[x, y] = (float)interpolator.Interpolate(v);
            }
         }


         interpolator.Clear();
         interpolator = null;
         return true;
      }



      override public void displayPropertiesDlg()
      {
         Device_CurvesDlg dlg = new Device_CurvesDlg(this);
         dlg.ShowDialog();
      }

   }

}