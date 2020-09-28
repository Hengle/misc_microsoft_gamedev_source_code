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
   [XmlRoot("CanvasNode")]
   public class CanvasNode
   {
      #region Members
      protected Point mLocation = new Point(10, 10);
      public Point Location
      {
         get { return mLocation; }
         set { mLocation = value; }
      }

      protected Guid mGUID = System.Guid.NewGuid();
      public Guid UniqueID
      {
         get { return mGUID; }
         set { mGUID = value; }
      }

      protected Size mSize = new Size(40, 20);
      [XmlIgnore]
      public Size Size
      {
         get { return mSize; }
         set { mSize = value; }
      }

      [XmlIgnore]
      public byte mDepthLayer =0;  

      protected int mBorderSize = 2;

      protected Color mColorBottom = Color.Yellow;
      protected Color mColorTop = Color.Red;


      private bool mIsSelected = false;
      [XmlIgnore]
      public bool IsSelected
      {
         get { return mIsSelected; }
         set { mIsSelected = value; }
      }

      private bool mIsVisible = true;
      [XmlIgnore]
      public bool IsVisible
      {
         get { return mIsVisible; }
         set { mIsVisible = value; }
      }

      private bool mIsEnabled = true;
      [XmlIgnore]
      public bool IsEnabled
      {
         get { return mIsEnabled; }
         set { mIsEnabled = value; }
      }

      private List<CanvasNode> mConnectedNeighbors = new List<CanvasNode>();
      [XmlIgnore]
      public List<CanvasNode> Neighbors
      {
         get
         {
            return mConnectedNeighbors;
         }
         set
         {
            mConnectedNeighbors = value;
         }
      }
      #endregion

      public CanvasNode()
      {

      }
      ~CanvasNode()
      {
      }

      protected GraphCanvas mOwningCanvas = null;

      
      public virtual void OnRemovedFromCanvas()
      {

      }

      public virtual void render(Graphics g)
      {
         if (!IsVisible)
            return;

         GDIStatic.Pen_Black.Width = mBorderSize;
         Rectangle rect = new Rectangle(mLocation, mSize);

         //fill our main background
         LinearGradientBrush lBrush = new LinearGradientBrush(rect, mColorTop, mColorBottom, LinearGradientMode.Vertical);
         g.FillRectangle(lBrush, rect);

         //draw our border
         g.DrawRectangle(GDIStatic.Pen_Black, rect);

          if (mIsSelected)
             drawSelectedRect(g);
      }
      void drawSelectedRect(Graphics g)
      {
         GDIStatic.Pen_Black.Color = Color.Yellow;
         GDIStatic.Pen_Black.Width = 2;
         GDIStatic.Pen_Black.DashStyle = DashStyle.Dash;

         //draw selected border
         Rectangle trect = new Rectangle(mLocation.X - mBorderSize, mLocation.Y - mBorderSize, mSize.Width + (mBorderSize * 3), mSize.Height + (mBorderSize * 3));

         g.DrawRectangle(GDIStatic.Pen_Black, trect);

         GDIStatic.Pen_Black.Color = Color.Black;
         GDIStatic.Pen_Black.Width = 1;
         GDIStatic.Pen_Black.DashStyle = DashStyle.Solid;  
      }


      #region ON action events
      public virtual void onClick(Point mousePoint, MouseEventArgs mouseEvent)
      {
      }

      public virtual void onMouseOver(Point mousePoint, MouseEventArgs mouseEvent)
      {
      }

      public virtual void onMouseDown(Point mousePoint, MouseEventArgs mouseEvent)
      {
      }

      public virtual void onMouseUp(Point mousePoint, MouseEventArgs mouseEvent)
      {
      }

      public virtual void onKeyDown(KeyEventArgs keyEvent)
      {
      }

      public virtual void onKeyUp(KeyEventArgs keyEvent)
      {
      }

      public virtual void draggedByMouse(Point prevMousePos, Point currMousePos)
      {
         Point diff = new Point(currMousePos.X - prevMousePos.X, currMousePos.Y - prevMousePos.Y);
         mLocation.X += diff.X;
         mLocation.Y += diff.Y;

      }

    

      #endregion

      public virtual bool isMouseOver(Point mousePoint)
      {
         if (!IsEnabled)
            return false;

         return (mousePoint.X > mLocation.X && mousePoint.Y > mLocation.Y &&
                 mousePoint.X < mLocation.X + mSize.Width && mousePoint.Y < mLocation.Y + mSize.Height);
      }

      public virtual bool isTouchesRect(int minX, int maxX, int minY, int maxY)
      {
          return isTouchesRect(minX, maxX, minY, maxY, false);
      }
      public virtual bool isTouchesRect(int minX, int maxX, int minY, int maxY,bool ignoreEnable)
      {
          if(!ignoreEnable)
            if (!IsEnabled)
                return false;

         return (mLocation.X < maxX && mLocation.Y < maxY &&
               minX < (mLocation.X + mSize.Width) && minY < (mLocation.Y + mSize.Height))
            ;
      }


      public virtual Point getConnectionLocation()
      {
         return mLocation;
      }
      
   };
}