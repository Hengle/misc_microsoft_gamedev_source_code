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

namespace graphapp
{




   public class Device_Constant : MaskDevice
   {
       
      MaskParam mConstraintMask = new MaskParam();
       [XmlIgnore]
      [ConnectionType("Constraint", "Constraint Input")]
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

       FloatParam mValue = new FloatParam(0, 0, 1);
        [ConnectionType("Param", "Value")]
       public float Value
        {
           get { return mValue.Value; }
           set { mValue.Value = value; }
        }


        

      public Device_Constant()
      {

      }
      public Device_Constant(GraphCanvas owningCanvas)
         :
         base(owningCanvas)
      {
         
         base.Text = "Constant";
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
          Device_Constant dc = fromNode as Device_Constant;
          mGUID = dc.mGUID;
          draggedByMouse(Location,dc.Location);
          

          Value = dc.Value;
          
          return true;
       }

      override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
          if (!verifyInputConnections())
              return false;

          gatherInputAndParameters(parms);

          MaskParam mp = ((MaskParam)(connPoint.ParamType));
          mp.Value = new DAGMask(parms.Width, parms.Height);
          mp.Value.mConstraintMask = ConstraintMask;
          mp.Value.setAllToValue(Value);
          
          
          
          return true;
      }

      
   }
}