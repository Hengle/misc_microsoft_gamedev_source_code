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

namespace graphapp
{
    public class ConnectionType : Attribute
    {
        public string ConnType;
        public string Description;
        public bool Required;
        public ConnectionType(string connectionType, string desc, bool req)
        {
            this.ConnType = connectionType;
            this.Description = desc;
            this.Required = req;
        }
        public ConnectionType(string connectionType, string desc)
        {
            this.ConnType = connectionType;
            this.Description = desc;
            this.Required = true;
        }
    }

   #region Connection Points
   public class ConnectionPoint : CanvasNode
   {
      protected int mConnectionOffset = 10;
       public ConnectionPoint(ParamType paramType, string Descriptor,MaskDAGGraphNode ownerMaskDAGGraphNode, GraphCanvas owningCanvas)
      {
         
         mDepthLayer = 1;
         mOwnerMaskDAGGraphNode = ownerMaskDAGGraphNode;
         mParamType = paramType;
         ID = Descriptor;

      }
      protected string mID = "Connection";
      public string ID
      {
         get 
         { 
             return mID; 
         }
         set 
         { 
             mID = value; 
         }
      }
      ParamType mParamType = null;
      public ParamType ParamType
      {
          get { return mParamType; }
      }

      protected MaskDAGGraphNode mOwnerMaskDAGGraphNode = null;
      public MaskDAGGraphNode OwnerNode
      {
          get { return mOwnerMaskDAGGraphNode; }
      }

      public override void onMouseOver(Point mousePoint, MouseEventArgs mouseEvent)
      {
          if (!IsEnabled)
              return;

          string rp = ParamType.Type.ToString();

          int lio = rp.LastIndexOf('.');
          if (lio != -1)
              rp = rp.Substring(lio+1);
          mOwnerMaskDAGGraphNode.setHelperText(mID + "(" + rp + ")");
      }

   }
   public class OutputConnectionPoint : ConnectionPoint
   {

       public OutputConnectionPoint(ParamType valueType, string Descriptor, MaskDAGGraphNode ownerMaskDAGGraphNode, GraphCanvas owningCanvas)
           : base(valueType, Descriptor, ownerMaskDAGGraphNode, owningCanvas)
      {
         Location = new Point(2, 2);
         mSize = new Size(10, 10);

         mBorderSize = 1;

         mColorBottom = Color.LightGray;
         mColorTop = Color.Gray;
      }

      public override Point getConnectionLocation()
      {

         Point p = new Point((int)((Location.X + (mSize.Width >> 1)) + mConnectionOffset), (int)((Location.Y + (mSize.Height >> 1))));
         return p;
      }
      

   }
   public class InputConnectionPoint : ConnectionPoint
   {
      bool mIsRequiredInput = true;
      public bool IsRequired
      {
         get { return mIsRequiredInput; }
         set { mIsRequiredInput = value; }
      }
      public InputConnectionPoint(ParamType valueType, bool requiredInput, string Descriptor, MaskDAGGraphNode ownerMaskDAGGraphNode, GraphCanvas owningCanvas)
          : base(valueType, Descriptor, ownerMaskDAGGraphNode, owningCanvas)
      {
         mLocation = new Point(2, 2);
         mSize = new Size(10, 10);

         mBorderSize = 1;

         mColorBottom = Color.Gray;
         mColorTop = Color.White;

         mIsRequiredInput = requiredInput;
      }

      public override Point getConnectionLocation()
      {
         Point p = new Point((int)((Location.X + (mSize.Width >> 1)) - mConnectionOffset), (int)((Location.Y + (mSize.Height >> 1))));
         return p;
      }

     
   }
   public class ParamConnectionPoint : ConnectionPoint
   {
       public ParamConnectionPoint(ParamType valueType, string Descriptor, MaskDAGGraphNode ownerMaskDAGGraphNode, GraphCanvas owningCanvas)
           : base(valueType, Descriptor, ownerMaskDAGGraphNode, owningCanvas)
      {
         Location = new Point(2, 2);
         mSize = new Size(6, 10);

         mBorderSize = 1;

         mColorBottom = Color.Purple;
         mColorTop = Color.Orange;
      }

      public override Point getConnectionLocation()
      {

         Point p = new Point((int)((Location.X + (mSize.Width >> 1))), (int)((Location.Y + (mSize.Height >> 1)) - mConnectionOffset));
         return p;
      }
   }
   public class ConstraintConnectionPoint : ConnectionPoint
   {

      public ConstraintConnectionPoint(ParamType valueType, string Descriptor, MaskDAGGraphNode ownerMaskDAGGraphNode, GraphCanvas owningCanvas)
         : base(valueType, Descriptor, ownerMaskDAGGraphNode, owningCanvas)
      {
         Location = new Point(2, 2);
         mSize = new Size(10, 10);

         mBorderSize = 1;

         mColorBottom = Color.Purple;
         mColorTop = Color.White;
      }

      public override Point getConnectionLocation()
      {

         Point p = new Point((int)((Location.X + (mSize.Width >> 1))), (int)((Location.Y + (mSize.Height >> 1)) + mConnectionOffset));
         return p;
      }


   }
   #endregion
}