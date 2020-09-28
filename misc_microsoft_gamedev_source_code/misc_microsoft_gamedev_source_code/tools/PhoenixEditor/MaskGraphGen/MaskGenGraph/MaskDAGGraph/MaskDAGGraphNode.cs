using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;

namespace graphapp
{
   public class MaskDAGGraphNode : CanvasNode
   {
      string mText = "MaskDAGGraphNode";
      protected string Text
      {
         get { return mText; }
         set { mText = value; }
      }

      string mDescription = "Generic Graph Node";
      string Description
      {
         get { return mDescription; }
         set { mDescription = value; }
      }

      protected int mNameOffset = 2;

      protected int mFontSize = 7;
      protected string mFontName = "Verdana";

      string mHelperText = null;

      protected List<OutputConnectionPoint> mOutputConnections = new List<OutputConnectionPoint>();
      protected List<InputConnectionPoint> mInputConnections = new List<InputConnectionPoint>();
      protected List<ParamConnectionPoint> mParamConnections = new List<ParamConnectionPoint>();
      protected List<ConstraintConnectionPoint> mConstraintConnections = new List<ConstraintConnectionPoint>();

      public MaskDAGGraphNode()
      {
      }

      public MaskDAGGraphNode(GraphCanvas owningCanvas)
      {
         mOwningCanvas = owningCanvas;

          //grab the point from the user
         Location = owningCanvas.transformPoint(owningCanvas.LastClickedPoint,owningCanvas.CanvasTransformInv);

      }
      ~MaskDAGGraphNode()
      {
      }

      public override void OnRemovedFromCanvas()
      {
         //remove my children from canvas as well.
         for (int i = 0; i < mOutputConnections.Count;i++ )
            mOwningCanvas.removeCanvasNode(mOutputConnections[i]);

         for (int i = 0; i < mInputConnections.Count; i++)
            mOwningCanvas.removeCanvasNode(mInputConnections[i]);

         for (int i = 0; i < mParamConnections.Count; i++)
            mOwningCanvas.removeCanvasNode(mParamConnections[i]);

         for (int i = 0; i < mConstraintConnections.Count; i++)
            mOwningCanvas.removeCanvasNode(mConstraintConnections[i]);


      }

      #region saveload
      public virtual bool save()
      {
         return false;
      }
      public virtual bool load(MaskDAGGraphNode fromNode)
      {
         mGUID = fromNode.mGUID;
         draggedByMouse(Location, fromNode.Location);
         return false;
      }


      

      public virtual List<DAGCanvas.MaskDAGConnectionXML> generateConnectionLists()
      {
         List<DAGCanvas.MaskDAGConnectionXML> cList = new List<DAGCanvas.MaskDAGConnectionXML>();

         Type type = GetType();// Get object type

         System.Reflection.PropertyInfo[] pi = type.GetProperties();
         for (int i = 0; i < pi.Length; i++)
         {
            System.Reflection.PropertyInfo prop = pi[i];

            object[] custAttrib = prop.GetCustomAttributes(false);
            if (custAttrib == null)
               continue;

            Type ptt = prop.PropertyType;
            for (int k = 0; k < custAttrib.Length; k++)
            {
               if (custAttrib[k] is ConnectionType)
               {
                  ConnectionType ct = custAttrib[k] as ConnectionType;
                  if (ct.ConnType == "Param")
                  {
                     for (int j = 0; j < mParamConnections.Count; j++)
                     {
                        if (mParamConnections[j].Neighbors.Count == 0)
                           continue;

                        if (mParamConnections[j].ID == ct.Description)
                        {
                           DAGCanvas.MaskDAGConnectionXML dcxl = new DAGCanvas.MaskDAGConnectionXML();
                           dcxl.mOwnerDeviceID = UniqueID;
                           dcxl.mConnectionName = mParamConnections[j].ID;
                           dcxl.mNeighborConnectionName = ((ConnectionPoint)mParamConnections[j].Neighbors[0]).ID;
                           dcxl.mNeighborDeviceID = ((ConnectionPoint)mParamConnections[j].Neighbors[0]).OwnerNode.UniqueID;
                           cList.Add(dcxl);
                        }
                     }
                  }
                  else if (ct.ConnType == "Input")
                  {
                     for (int j = 0; j < mInputConnections.Count; j++)
                     {
                        if (mInputConnections[j].Neighbors.Count == 0)
                           continue;

                        if (mInputConnections[j].ID == ct.Description)
                        {
                           DAGCanvas.MaskDAGConnectionXML dcxl = new DAGCanvas.MaskDAGConnectionXML();
                           dcxl.mOwnerDeviceID = UniqueID;
                           dcxl.mConnectionName = mInputConnections[j].ID;
                           dcxl.mNeighborConnectionName = ((ConnectionPoint)mInputConnections[j].Neighbors[0]).ID;
                           dcxl.mNeighborDeviceID = ((ConnectionPoint)mInputConnections[j].Neighbors[0]).OwnerNode.UniqueID;
                           cList.Add(dcxl);
                        }
                     }
                  }
                  else if (ct.ConnType == "Output")
                  {
                     for (int j = 0; j < mOutputConnections.Count; j++)
                     {
                        if (mOutputConnections[j].Neighbors.Count == 0)
                           continue;

                        if (mOutputConnections[j].ID == ct.Description)
                        {
                           DAGCanvas.MaskDAGConnectionXML dcxl = new DAGCanvas.MaskDAGConnectionXML();
                           dcxl.mOwnerDeviceID = UniqueID;
                           dcxl.mConnectionName = mOutputConnections[j].ID;
                           dcxl.mNeighborConnectionName = ((ConnectionPoint)mOutputConnections[j].Neighbors[0]).ID;
                           dcxl.mNeighborDeviceID = ((ConnectionPoint)mOutputConnections[j].Neighbors[0]).OwnerNode.UniqueID;
                           cList.Add(dcxl);
                        }
                     }
                  }
                  else if (ct.ConnType == "Constraint")
                  {
                     for (int j = 0; j < mConstraintConnections.Count; j++)
                     {
                        if (mConstraintConnections[j].Neighbors.Count == 0)
                           continue;

                        if (mConstraintConnections[j].ID == ct.Description)
                        {
                           DAGCanvas.MaskDAGConnectionXML dcxl = new DAGCanvas.MaskDAGConnectionXML();
                           dcxl.mOwnerDeviceID = UniqueID;
                           dcxl.mConnectionName = mConstraintConnections[j].ID;
                           dcxl.mNeighborConnectionName = ((ConnectionPoint)mConstraintConnections[j].Neighbors[0]).ID;
                           dcxl.mNeighborDeviceID = ((ConnectionPoint)mConstraintConnections[j].Neighbors[0]).OwnerNode.UniqueID;
                           cList.Add(dcxl);
                        }
                     }
                  }
               }
            }
         }

         return cList;
      }

      #endregion
      #region creation for children
      protected void addOutputConnectionPoint(ParamType pt, string name)
      {
          OutputConnectionPoint op = new OutputConnectionPoint(pt, name,this, mOwningCanvas);
         mOutputConnections.Add(op);
         mOwningCanvas.addCanvasNode(op);
      }
      protected void addInputConnectionPoint(ParamType pt, string name, bool requiredInput)
      {
          InputConnectionPoint op = new InputConnectionPoint(pt, requiredInput, name, this, mOwningCanvas);
         mInputConnections.Add(op);

         mOwningCanvas.addCanvasNode(op);
      }
      protected void addConstraintConnectionPoint(ParamType pt, string name)
      {
         ConstraintConnectionPoint op = new ConstraintConnectionPoint(pt, name, this, mOwningCanvas);
         mConstraintConnections.Add(op);

         mOwningCanvas.addCanvasNode(op);
      }
      protected void addParamConnectionPoint(ParamType pt, string name)
      {
          ParamConnectionPoint op = new ParamConnectionPoint(pt, name, this, mOwningCanvas);
         mParamConnections.Add(op);
         mOwningCanvas.addCanvasNode(op);
      }


      protected void generateConnectionPoints()
      {
          List<string> usedDescriptions = new List<string>();

          //try to do this dynamically...
          Type type = GetType();// Get object type

          System.Reflection.PropertyInfo[] pi = type.GetProperties();
          for (int i = 0; i < pi.Length; i++)
          {
              System.Reflection.PropertyInfo prop = pi[i];

              object[] custAttrib = prop.GetCustomAttributes(false);
              if (custAttrib == null)
                  continue;

              ParamType pt = null;
              Type ptt = prop.PropertyType;
              if (ptt == typeof(float)) pt = new FloatParam();
              else if (ptt == typeof(int)) pt = new IntParam();
              else if (ptt == typeof(bool)) pt = new BoolParam();
              else if (ptt == typeof(DAGMask)) pt = new MaskParam();

              for (int k = 0; k < custAttrib.Length; k++)
              {
                  if (custAttrib[k] is ConnectionType)
                  {
                      ConnectionType ct = custAttrib[k] as ConnectionType;

                      //make sure this connection is unique
                      for (int j = 0; j < usedDescriptions.Count; j++)
                      {
                          if (ct.Description == usedDescriptions[j])
                          {
                              MessageBox.Show(type.FullName + " has already defined a connection value with description " + ct.Description);
                              System.Diagnostics.Debug.Assert(false);
                              return;
                          }

                      }

                      if (ct.ConnType == "Param")
                      {
                          addParamConnectionPoint(pt, ct.Description);
                      }
                      else if (ct.ConnType == "Input")
                      {
                          addInputConnectionPoint(pt, ct.Description, ct.Required);
                      }
                      else if (ct.ConnType == "Output")
                      {
                          addOutputConnectionPoint(pt, ct.Description);
                      }
                      else if (ct.ConnType == "Constraint")
                      {
                         addConstraintConnectionPoint(pt, ct.Description);
                      }
                      else
                      {
                          System.Diagnostics.Debug.Assert(false,"Unrecognized connection type listed for object " + type.FullName);
                      }

                      usedDescriptions.Add(ct.Description);
                      break;
                  }
              }
          }
      }
      protected void resizeFromConnections()
      {
         int outputBlockSpacing = 4;
         int inputBlockSpacing = 4;
         int paramBlockSpacing = 4;

         int inputBlockWidth = 10;
         int outputBlockWidth = 10;
         int paramBlockWidth = 6;
         int constraintBlockWidth = 10;

         int spaceToParamStart = 20;
         int spaceToOutputStart = 20;

         //calculate Size data
         mSize.Width = inputBlockWidth +
                       spaceToParamStart +
                       ((paramBlockSpacing + paramBlockWidth) * mParamConnections.Count) +
                       spaceToOutputStart +
                       outputBlockWidth;

         int maxHeight = Math.Max(mInputConnections.Count, mOutputConnections.Count);

         mSize.Height = inputBlockSpacing + inputBlockSpacing +
                       ((inputBlockSpacing + inputBlockWidth) * maxHeight);


        
      


         //reset the connection positions
         for (int i = 0; i < mOutputConnections.Count; i++)
         {
            int x = Location.X + mSize.Width - outputBlockWidth;
            int y = Location.Y + outputBlockSpacing + ((outputBlockSpacing + outputBlockWidth) * i);
            mOutputConnections[i].Location = new Point(x, y);
         }
         for (int i = 0; i < mInputConnections.Count; i++)
         {
            int x = Location.X;
            int y = Location.Y + inputBlockSpacing + ((inputBlockSpacing + inputBlockWidth) * i);
            mInputConnections[i].Location = new Point(x, y);
         }
         for (int i = 0; i < mParamConnections.Count; i++)
         {
            int x = Location.X + inputBlockWidth + spaceToParamStart + ((paramBlockSpacing + paramBlockWidth) * i);
            int y = Location.Y;

            mParamConnections[i].Location = new Point(x, y);
         }
         for (int i = 0; i < mConstraintConnections.Count; i++)
         {
            int x = Location.X + inputBlockWidth + spaceToParamStart + ((paramBlockSpacing + paramBlockWidth) * i);
            int y = Location.Y + mSize.Height - constraintBlockWidth;

            mConstraintConnections[i].Location = new Point(x, y);
         }
         
      }

      public bool gatherInputAndParameters(OutputGenerationParams parms)
      {
          //walk all of our local params
          //find the associated connection point
          //copy the data from the connection point to our local point

          Type type = GetType();// Get object type
          System.Reflection.PropertyInfo[] pi = type.GetProperties();
          for (int i = 0; i < pi.Length; i++)
          {
              System.Reflection.PropertyInfo prop = pi[i];

              object[] custAttrib = prop.GetCustomAttributes(false);
              if (custAttrib == null)
                  continue;

              Type ptt = prop.PropertyType;

              for (int k = 0; k < custAttrib.Length; k++)
              {
                  if (custAttrib[k] is ConnectionType)
                  {
                      ConnectionType ct = custAttrib[k] as ConnectionType;
                      if (ct.ConnType == "Param")
                      {
                          for (int j = 0; j < mParamConnections.Count; j++)
                          {
                              if (mParamConnections[j].Neighbors.Count == 0)
                                  continue;

                              if (mParamConnections[j].ID == ct.Description)
                              {
                                 ConnectionPoint CP = mParamConnections[j].Neighbors[0] as ConnectionPoint;
                                 MaskDAGGraphNode GN = CP.OwnerNode as MaskDAGGraphNode;
                                 if (!GN.computeOutput(mParamConnections[j], parms))
                                    return false;

                                  if (ptt == typeof(float))
                                      prop.SetValue(this, ((FloatParam)mParamConnections[j].ParamType).Value, null);
                                  else if (ptt == typeof(int))
                                      prop.SetValue(this, ((IntParam)mParamConnections[j].ParamType).Value, null);
                                  else if (ptt == typeof(bool))
                                      prop.SetValue(this, ((BoolParam)mParamConnections[j].ParamType).Value, null);
                                  else if (ptt == typeof(DAGMask))
                                      prop.SetValue(this, ((MaskParam)mParamConnections[j].ParamType).Value, null);
                              }
                          }
                      }
                      else if (ct.ConnType == "Input")
                      {
                          for (int j = 0; j < mInputConnections.Count; j++)
                          {
                              if (mInputConnections[j].ID == ct.Description)
                              {
                                  ConnectionPoint CP = mInputConnections[j].Neighbors[0] as ConnectionPoint;
                                  MaskDAGGraphNode GN = CP.OwnerNode as MaskDAGGraphNode;
                                  if(!GN.computeOutput(mInputConnections[j], parms))
                                    return false;


                                  if (ptt == typeof(float))
                                      prop.SetValue(this, ((FloatParam)mInputConnections[j].ParamType).Value, null);
                                  else if (ptt == typeof(int))
                                      prop.SetValue(this, ((IntParam)mInputConnections[j].ParamType).Value, null);
                                  else if (ptt == typeof(bool))
                                      prop.SetValue(this, ((BoolParam)mInputConnections[j].ParamType).Value, null);
                                  else if (ptt == typeof(DAGMask))
                                      prop.SetValue(this, ((MaskParam)mInputConnections[j].ParamType).Value, null);
                              }
                          }
                      }
                      else if (ct.ConnType == "Constraint")
                      {
                         for (int j = 0; j < mConstraintConnections.Count; j++)
                         {
                            if (mConstraintConnections[j].Neighbors.Count == 0)
                            {
                               
                               if (ptt == typeof(float))
                                  prop.SetValue(this, 0, null);
                               else if (ptt == typeof(int))
                                  prop.SetValue(this, 0, null);
                               else if (ptt == typeof(bool))
                                  prop.SetValue(this, false, null);
                               else if (ptt == typeof(DAGMask))
                                  prop.SetValue(this, null, null);
                            }
                            else if (mConstraintConnections[j].ID == ct.Description)
                            {
                               ConnectionPoint CP = mConstraintConnections[j].Neighbors[0] as ConnectionPoint;
                               MaskDAGGraphNode GN = CP.OwnerNode as MaskDAGGraphNode;
                               if (!GN.computeOutput(mConstraintConnections[j], parms))
                                  return false;


                               if (ptt == typeof(float))
                                  prop.SetValue(this, ((FloatParam)mConstraintConnections[j].ParamType).Value, null);
                               else if (ptt == typeof(int))
                                  prop.SetValue(this, ((IntParam)mConstraintConnections[j].ParamType).Value, null);
                               else if (ptt == typeof(bool))
                                  prop.SetValue(this, ((BoolParam)mConstraintConnections[j].ParamType).Value, null);
                               else if (ptt == typeof(DAGMask))
                                  prop.SetValue(this, ((MaskParam)mConstraintConnections[j].ParamType).Value, null);
                            }
                         }
                      }

                     
                      break;
                  }
              }
          }
          return true;
      }

      
      public void setHelperText(string text)
      {
         mHelperText = text;
      }

      virtual public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
      {
          return false;
      }

      virtual public void displayPropertiesDlg()
      {
          Device_PropertyDlg dpd = new Device_PropertyDlg();
          dpd.StartPosition = FormStartPosition.CenterScreen;
          dpd.initalize(this);
          dpd.ShowDialog();
      }

      #endregion

      public override void render(Graphics g)
      {
         if (!IsVisible)
            return;

         //draw drop shadow
         Point shadowOffset = new Point(Location.X + 5, Location.Y + 5);
         Rectangle sRect = new Rectangle(shadowOffset, mSize);
         SolidBrush sBrush = new SolidBrush(Color.FromArgb(128, 0, 0, 0));
         g.FillRectangle(sBrush, sRect);

         //render our origional box
         base.render(g);

         //draw our name
         Font f = new Font(mFontName, mFontSize);
         SolidBrush b = verifyInputConnections()?new SolidBrush(Color.Black): new SolidBrush(Color.Red);
         g.DrawString(Text, f, b, Location.X - mBorderSize * 2, Location.Y + mSize.Height + mBorderSize + mNameOffset);

         if(mHelperText!=null)
         {
            int charWidth = mFontSize;
            int startOffset = (mHelperText.Length >> 1) *charWidth;

            int mHelperStringCenterX = Location.X + (mSize.Width >> 1) - startOffset;
            int mHelperStringCenterY = Location.Y - (mFontSize * 2);

            g.DrawString(mHelperText, f, b, mHelperStringCenterX, mHelperStringCenterY);
         }

         //render children here
         for (int i = 0; i < mInputConnections.Count; i++)
            mInputConnections[i].render(g);
         for (int i = 0; i < mOutputConnections.Count; i++)
            mOutputConnections[i].render(g);
         for (int i = 0; i < mParamConnections.Count; i++)
            mParamConnections[i].render(g);
         for (int i = 0; i < mConstraintConnections.Count; i++)
            mConstraintConnections[i].render(g);
         
      }

      public void generatePreview()
      {
          ((DAGCanvas)mOwningCanvas).generatePreview(this);
      }
      protected bool verifyInputConnections()
      {
         for (int i = 0; i < mInputConnections.Count; i++)
            if (mInputConnections[i].IsRequired && mInputConnections[i].Neighbors.Count==0)
               return false;

         return true;


      }

      #region ON action events
      public override void onClick(Point mousePoint, MouseEventArgs mouseEvent)
      {
         if (!IsEnabled)
            return;
      }

      public override void onMouseOver(Point mousePoint, MouseEventArgs mouseEvent)
      {
         if (!IsEnabled)
            return;

      }

      public override void onMouseDown(Point mousePoint, MouseEventArgs mouseEvent)
      {
         if (!IsEnabled)
            return;

         
      }

      public override void onMouseUp(Point mousePoint, MouseEventArgs mouseEvent)
      {
         if (!IsEnabled)
            return;

         generatePreview();
      }

      public override void onKeyDown(KeyEventArgs keyEvent)
      {
      }

      public override void onKeyUp(KeyEventArgs keyEvent)
      {
         if (!IsEnabled)
            return;
      }

      public override void draggedByMouse(Point prevMousePos, Point currMousePos)
      {
         Point diff = new Point(currMousePos.X - prevMousePos.X, currMousePos.Y - prevMousePos.Y);
         mLocation.X += diff.X;
         mLocation.Y += diff.Y;

         for (int i = 0; i < mOutputConnections.Count; i++)
         {
            Point p = mOutputConnections[i].Location;
            p.Offset(diff.X, diff.Y);
            mOutputConnections[i].Location = p;
         }
         for (int i = 0; i < mInputConnections.Count; i++)
         {
            Point p = mInputConnections[i].Location;
            p.Offset(diff.X, diff.Y);
            mInputConnections[i].Location = p;
         }
         for (int i = 0; i < mParamConnections.Count; i++)
         {
            Point p = mParamConnections[i].Location;
            p.Offset(diff.X, diff.Y);
            mParamConnections[i].Location = p;
         }
         for (int i = 0; i < mConstraintConnections.Count; i++)
         {
            Point p = mConstraintConnections[i].Location;
            p.Offset(diff.X, diff.Y);
            mConstraintConnections[i].Location = p;
         }
         
      }

      #endregion

   }
}