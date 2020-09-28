using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.Text;
using System.Windows.Forms;
using System.IO;

using System.Xml;
using System.Xml.Serialization;

using EditorCore;
using NoiseGeneration;

namespace graphapp
{

   public class GraphCanvas
   {
      Size mSize = new Size(256, 256);

      protected List<CanvasNode> mNodes = new List<CanvasNode>();

      enum eMode
      {
         eModeNone = 0,
         eModeConnection,

      }
      eMode mMode = eMode.eModeNone;

      AngledConnectionLine mTempConnectionLineAngled = new AngledConnectionLine();
      ConnectionLine mTempConnectionLine = new ConnectionLine();

      #region file management
      public void newCanvas()
      {
         mMode = eMode.eModeNone;
         unfilterForConnection();
      }
      public void cleanCanvas()
      {
         removeAllNodes();
         resetTransform();
      }

      public virtual bool saveCanvasToDisk(string filename)
      {
         return false;
      }
      public virtual bool loadCanvasFromDisk(string filename)
      {
         return false;
      }

      public virtual bool saveCanvasToMemoryStream(MemoryStream ms)
      {
         return false;
      }

      public virtual bool loadCanvasFromMemoryStream(MemoryStream ms)
      {
         return false;
      }
      #endregion

      #region canvas Management
      public GraphCanvas(int width, int height)
      {
         mSize.Width = width;
         mSize.Height = height;



         InitializeComponent();
      }
      #endregion

      #region RENDERING
      public void render(Graphics g)
      {
         //draw our striped background
          drawBackground(g);

          g.Transform = mTransformMat;
     
         //draw the graph
         drawNodeLayer(g,0);
         drawConnectionLines(g);


         if(mMode == eMode.eModeNone)
         {
            if (mDragging && !mClickedOnGraphObject)
            {
               drawDragBox(g);
            }
         }
         else if (mMode == eMode.eModeConnection)
         {
            drawTempConnectionLine(g);

         }
      }

      

      void drawBackground(Graphics g)
      {
          //fill our main background
          Rectangle rect = new Rectangle(new Point(0, 0), mSize);
          g.FillRectangle(GDIStatic.SolidBrush_lightGray, rect);

          //draw our grid lines
          int gridStep = 20;
          int numX = mSize.Width / gridStep;
          int numY = mSize.Height / gridStep;
          for (int x = 0; x < numX; x++)
              g.DrawLine(GDIStatic.Pen_DimGray, x * gridStep, 0, x * gridStep, mSize.Height);
          for (int y = 0; y < numY; y++)
              g.DrawLine(GDIStatic.Pen_DimGray, 0, y * gridStep, mSize.Width, y * gridStep);
      }

      void drawNodeLayer(Graphics g, int layerIndex)
      {
          //int winMinX = 0;
          //int winMinY = 0;
          //int winMaxX = mSize.Width;
          //int winMaxY = mSize.Height;

          foreach (CanvasNode gn in mNodes)
          {
              if (gn.mDepthLayer == layerIndex)
              {
               //   if(gn.isTouchesRect(winMinX,winMaxX,winMinY,winMaxY,true))
                    gn.render(g);
              }
          }
      }
      void drawConnectionLines(Graphics g)
      {
          //int winMinX = 0;
          //int winMinY = 0;
          //int winMaxX = mSize.Width;
          //int winMaxY = mSize.Height;

         foreach (CanvasNode gn in mNodes)
         {
            if (gn.Neighbors == null)
               continue;
            for (int i = 0; i < gn.Neighbors.Count; i++)
            {
               if (gn.Neighbors[i]==null )
                  continue;

               Point StartP = gn.getConnectionLocation();
               Point EndP = gn.Neighbors[i].getConnectionLocation();

               mTempConnectionLineAngled.setStartPoint(StartP.X, StartP.Y);
               mTempConnectionLineAngled.setEndPoint(EndP.X, EndP.Y);
             //  if (mTempConnectionLineAngled.isTouchesRect(winMinX, winMaxX, winMinY, winMaxY))
                  mTempConnectionLineAngled.render(g);

               //now just render a temp line from the connection point to the node center
               mTempConnectionLine.setStartPoint(StartP.X,StartP.Y);
               mTempConnectionLine.setEndPoint(gn.Location.X + (gn.Size.Width >> 1), gn.Location.Y + (gn.Size.Height >> 1));
               mTempConnectionLine.render(g);

               mTempConnectionLine.setStartPoint(EndP.X, EndP.Y);
               mTempConnectionLine.setEndPoint(gn.Neighbors[i].Location.X + (gn.Neighbors[i].Size.Width >> 1), gn.Neighbors[i].Location.Y + (gn.Neighbors[i].Size.Height >> 1));
               mTempConnectionLine.render(g);

            }
         }
      }
      void drawDragBox(Graphics g)
      {
         Point currPt = transformPoint(mCurrentPoint, mTransformMatInv);
         Point clickedPt = transformPoint(mClickedPoint, mTransformMatInv);
          

            

            //draw dotted line between clicked location and our location
            GDIStatic.Pen_Black.Width = 2;
            GDIStatic.Pen_Black.DashStyle = DashStyle.Dash;

            int minX = Math.Min(currPt.X, clickedPt.X);
            int maxX = Math.Max(currPt.X, clickedPt.X);
            int minY = Math.Min(currPt.Y, clickedPt.Y);
            int maxY = Math.Max(currPt.Y, clickedPt.Y);
            
            Rectangle drect = new Rectangle(minX,minY,maxX-minX,maxY-minY);
            g.DrawRectangle(GDIStatic.Pen_Black, drect);

            GDIStatic.Pen_Black.Width = 1;
            GDIStatic.Pen_Black.DashStyle = DashStyle.Solid;
      }
      void drawTempConnectionLine(Graphics g)
      {


         Point currPt = transformPoint(mCurrentPoint, mTransformMatInv);
         Point clickedPt = transformPoint(mClickedPoint, mTransformMatInv);
         //snapping
         CanvasNode selectedNode = intersectNodeMouseLoc(mCurrentPoint);
         if (selectedNode != null)
         {
            currPt = selectedNode.Location;
            currPt.X += selectedNode.Size.Width >> 1;
            currPt.Y += selectedNode.Size.Height >> 1;
         }

         //draw connection line..
         

         mTempConnectionLineAngled.setStartPoint(clickedPt.X, clickedPt.Y);
         mTempConnectionLineAngled.setEndPoint(currPt.X, currPt.Y);
         mTempConnectionLineAngled.render(g);
      }
      #endregion

      #region add RemoveNodes
      public void addCanvasNode(CanvasNode cn)
      {
         mNodes.Add(cn);
      }
      public void removeCanvasNode(CanvasNode cn)
      {
         cn.OnRemovedFromCanvas();
         removeConnections(cn);
         mNodes.Remove(cn);
      }
      protected void addConnection(CanvasNode a, CanvasNode b)
      {
         a.Neighbors.Add(b);
         b.Neighbors.Add(a);
      }
      protected void removeConnections(CanvasNode n)
      {
         if (n != null && n.Neighbors != null)
         {
             for(int i=0;i<n.Neighbors.Count;i++)
             {
                 CanvasNode cn = n.Neighbors[i];

                 if (cn == null || cn.Neighbors == null)
                     continue;

                 cn.Neighbors.Remove(n);
             }
             
            n.Neighbors.Clear();
         }
      }

      protected void removeAllNodes()
      {
         for (int i = 0; i < mNodes.Count; i++)
         {
            removeCanvasNode(mNodes[i]);
            i--;
         }
         mNodes.Clear();
      }
      protected void removeAllConnections()
      {
          foreach (CanvasNode cn in mNodes)
          {
              removeConnections(cn);
          }
      }
      #endregion

      #region action events

      #region selection
      void deselectAllNodes()
      {
         foreach (CanvasNode gn in mNodes)
         {
            gn.IsSelected = false;
         }
      }
      void selectNodesInDragRect()
      {
         Point currPt = transformPoint(mCurrentPoint, mTransformMatInv);
         Point clickedPt = transformPoint(mClickedPoint, mTransformMatInv);

         int minX = Math.Min(currPt.X, clickedPt.X);
         int maxX = Math.Max(currPt.X, clickedPt.X);
         int minY = Math.Min(currPt.Y, clickedPt.Y);
         int maxY = Math.Max(currPt.Y, clickedPt.Y);

         foreach (CanvasNode gn in mNodes)
         {
            if (gn.mDepthLayer == 0 && gn.isTouchesRect(minX, maxX, minY, maxY))
            {
               gn.IsSelected = true;
            }
         }
      }
      protected CanvasNode intersectNodeMouseLoc(Point mousePoint)
      {
         return intersectNodeMouseLoc(mousePoint, -1);
      }
      protected CanvasNode intersectNodeMouseLoc(Point mousePoint, int targetDepth)
      {
         Point currPt = transformPoint(mousePoint, mTransformMatInv);
         CanvasNode selectedNode = null;
         foreach (CanvasNode gn in mNodes)
         {
            if (gn.isMouseOver(currPt))
            {
               if (targetDepth != -1)
                  if (gn.mDepthLayer != targetDepth)
                     continue;


               if (selectedNode != null)
               {
                  if (gn.mDepthLayer > selectedNode.mDepthLayer)
                     selectedNode = gn;
               }
               else
               {
                  selectedNode = gn;
               }
            }
         }
         return selectedNode;
      }
      #endregion

   
      #region connection filtering
      protected virtual void filterForConnection(CanvasNode node,connectionRule rule)
      {
         foreach (CanvasNode gn in mNodes)
         {
            if (rule.isInvalidType(gn.GetType()))
            {
               gn.IsVisible = false;
               gn.IsEnabled = false;
            }
         }
      }
      void unfilterForConnection()
      {
         setAllVisible();
         setAllEnabled();
      }
      void setAllVisible()
      {
         foreach (CanvasNode gn in mNodes)
         {
            gn.IsVisible = true;
         }
      }
      void setAllEnabled()
      {
         foreach (CanvasNode gn in mNodes)
         {
            gn.IsEnabled = true;
         }
      }
      #endregion

      
      #region canvas moving (zoom / pan)
      System.Drawing.Drawing2D.Matrix mTransformMat = new Matrix();
      System.Drawing.Drawing2D.Matrix mTransformMatInv = new Matrix();
      ClampedFloat mZoomAmt = new ClampedFloat(0.4f, 1.4f, 1.0f);
      Point mTranslation = new Point(0, 0);
      public System.Drawing.Drawing2D.Matrix CanvasTransform
      {
         get { return mTransformMat; }
      }
      public System.Drawing.Drawing2D.Matrix CanvasTransformInv
      {
         get { return mTransformMatInv; }
      }

      void resetTransform()
      {
         mZoomAmt.Value = 1;
         mTranslation.X = 0;
         mTranslation.Y = 0;
         generateTransformMat();
      }
      void generateTransformMat()
      {
         mTransformMat = new Matrix();
         mTransformMat.Translate(mTranslation.X, mTranslation.Y);
         mTransformMat.Scale(mZoomAmt.Value, mZoomAmt.Value, MatrixOrder.Append);

         mTransformMatInv = mTransformMat.Clone();
         mTransformMatInv.Invert();
      }
      void translateCanvas(Point prevMousePos, Point currMousePos)
      {
         mTranslation.X += currMousePos.X - prevMousePos.X;
         mTranslation.Y += currMousePos.Y - prevMousePos.Y;
          generateTransformMat();
      }

      void zoomCanvas( Point location, int amount)
      {
          int numClicks = amount / 120;
          float zoomFactorPerClick = .1f;
          float zoomScale = zoomFactorPerClick * numClicks;
          mZoomAmt.Value += zoomScale;
          generateTransformMat();
      }
      public Point transformPoint(Point pt, Matrix transform)
      {
         Point[] pts = new Point[] { pt };
         transform.TransformPoints(pts);
         return pts[0];
      }
      #endregion

      #region UI
      bool mMouseIsDown = false; //updated on MOUSEDOWN
      bool mDragging = false;

      bool mPanning = false;    //for window translation

      Point mClickedPoint; //updated on MOUSEDOWN
      Point mCurrentPoint; //updated per mouse move
      bool mClickedOnGraphObject = false;
      CanvasNode mPrevSelectedNode = null;

      public Point LastClickedPoint
      {
          get { return mClickedPoint; }
      }
     


      
      public virtual void onMouseMove(Point mousePoint, MouseEventArgs mouseEvent)
      {
         if(mMode == eMode.eModeNone)
         {
            if (mMouseIsDown)
            {
                if (mouseEvent.Button == MouseButtons.Left)
                {
                    mDragging = true;
                    if (mClickedOnGraphObject)
                    {
                        foreach (CanvasNode gn in mNodes)
                        {
                            if (gn.mDepthLayer == 0 && gn.IsSelected)
                            {
                                gn.draggedByMouse(mCurrentPoint, mouseEvent.Location);
                            }
                        }
                    }
                }
                else if (mouseEvent.Button == MouseButtons.Middle)
                {
                    mPanning = true;
                    //we're panning
                    translateCanvas(mCurrentPoint, mouseEvent.Location);
                }
            }
            else
            {
               CanvasNode selectedNode = intersectNodeMouseLoc(mouseEvent.Location);
               if (selectedNode != null)
                  selectedNode.onMouseOver(mousePoint, mouseEvent);
            }

         }


         mCurrentPoint = mouseEvent.Location;


      }

      public virtual void onMouseDown(Point mousePoint, MouseEventArgs mouseEvent)
      {
         mMouseIsDown = true;
         mDragging = false;
         mClickedPoint = mouseEvent.Location;
         mCurrentPoint = mouseEvent.Location;
         mClickedOnGraphObject = false;

         //find the node intersecting with the mouse
         CanvasNode selectedNode = intersectNodeMouseLoc(mClickedPoint);

         if (mMode == eMode.eModeNone)
         {
             if (selectedNode != null)
             {
                 mClickedOnGraphObject = true;
                 if (mouseEvent.Button == MouseButtons.Left)
                 {
                     //if this node isn't select it, then clear, and select it
                     if (!selectedNode.IsSelected)
                     {
                         deselectAllNodes();
                         selectedNode.IsSelected = true;

                         //is this a connection type node? if so, put us in connection mode!
                         int connectionIndex = connectionTypeIndex(selectedNode);
                         if (connectionIndex != -1)
                         {
                             removeConnections(selectedNode);
                             mMode = eMode.eModeConnection;
                             filterForConnection(selectedNode, mConnectionRules[connectionIndex]);
                             selectedNode.IsEnabled = true;
                             selectedNode.IsVisible = true;
                             mPrevSelectedNode = selectedNode;
                         }
                     }
                 }
                 else
                 {
                     //per node context menu plz..
                 }

                 selectedNode.onMouseDown(mCurrentPoint, mouseEvent);
             }
             else
             {
                 if (mouseEvent.Button == MouseButtons.Right)
                 {
                 }
             }
         }
         else
         {
            if (selectedNode == null)
            {
               mMode = eMode.eModeNone;
               unfilterForConnection();
            }
            else
            {
               mClickedOnGraphObject = true;
               if (mouseEvent.Button == MouseButtons.Left)
               {
                  removeConnections(selectedNode);

                  //connect us
                  addConnection(mPrevSelectedNode, selectedNode);
                  mPrevSelectedNode = null;

                  deselectAllNodes();
               }
               
               mMode = eMode.eModeNone;
               unfilterForConnection();
            }
         }


         selectedNode = null;
      }

      public virtual void onMouseUp(Point mousePoint, MouseEventArgs mouseEvent)
      {
         mMouseIsDown = false;

         //if we're dragging
         if (mDragging)
         {
            mCurrentPoint = mouseEvent.Location;

            //if we wern't dragging an object, then we were in selection
            if (!mClickedOnGraphObject)
               selectNodesInDragRect();
            
            
            mDragging = false;
            return;  //don't continue from here.
         }



         //if we DIDN'T click on an object
         if (!mClickedOnGraphObject)
         {
             if (mouseEvent.Button == MouseButtons.Right)
             {
                 if (mPanning)
                 {
                     mPanning = false;
                 }
                 else
                 {
                     //clear all nodes and show context menu
                     mClickedPoint = mouseEvent.Location;
                     deselectAllNodes();
                     contextMenuStrip1.Show(mousePoint);
                 }
             }

             if (mouseEvent.Button == MouseButtons.Left)
             {
                 //just clear all nodes
                 deselectAllNodes();
             }
         }
         else
         {
             

             CanvasNode selectedNode = intersectNodeMouseLoc(mouseEvent.Location);
             if(selectedNode!=null)selectedNode.onMouseUp(mouseEvent.Location, mouseEvent);
         }
         
      }

      public virtual void onDoubleClick(Point mousePoint, MouseEventArgs mouseEvent)
      {
          
      }

      public virtual void onKeyDown(KeyEventArgs keyEvent)
      {
      }

      public virtual void onKeyUp(KeyEventArgs keyEvent)
      {
         if(keyEvent.KeyCode == Keys.Delete)
         {
            List<CanvasNode> itemsToDelete = new List<CanvasNode>();
            for(int i=0;i<mNodes.Count;i++)
            {
               if (mNodes[i].IsSelected)
                  itemsToDelete.Add(mNodes[i]);
            }
            if (itemsToDelete.Count == 0)
               return;

            string msgString = "";
            if(itemsToDelete.Count ==1)
               msgString = "Are you sure you want to delete this item? This cannot be Undone!!";
            else
               msgString = "Are you sure you want to delete these " + itemsToDelete.Count + " items? This cannot be Undone!!";



            if (MessageBox.Show(msgString, "Deletion Warning", MessageBoxButtons.OKCancel,MessageBoxIcon.Exclamation) == DialogResult.OK)
            {
               //remove them all from the connection graph first
               for(int i=0;i<itemsToDelete.Count;i++)
                  removeCanvasNode(itemsToDelete[i]);
            }
            itemsToDelete.Clear();
         }
         else if (keyEvent.KeyCode == Keys.Escape)
         {
             deselectAllNodes();
             mMode = eMode.eModeNone;
             unfilterForConnection();
         }
      }

      public virtual void onMouseScroll(MouseEventArgs ScrollEvent)
      {
          zoomCanvas(ScrollEvent.Location,ScrollEvent.Delta);
      }

      #endregion

      #endregion

      #region connectionData
      public class connectionRule
      {
         public Type mTargetType;
         public Type[] mInvalidConnectType = null;
         public Type[] mValidConnectType = null;

         public bool isInvalidType(Type objT)
         {
            for (int i = 0; i < mInvalidConnectType.Length; i++)
            {
               if (mInvalidConnectType[i] == objT)
                  return true;
            }
               return false;
         }

         public bool isValidType(Type objT)
         {
            for (int i = 0; i < mValidConnectType.Length; i++)
            {
               if (mValidConnectType[i] == objT)
                  return true;
            }
            return false;
         }
      };

      protected List<connectionRule> mConnectionRules = new List<connectionRule>();
      public void addConnectionRule(Type targetType, Type[] validConnectionTypes, Type[] invalidConnectionTypes)
      {
         connectionRule cr = new connectionRule();
         cr.mTargetType = targetType;

         cr.mInvalidConnectType = new Type[invalidConnectionTypes.Length];
         invalidConnectionTypes.CopyTo(cr.mInvalidConnectType,0);

         cr.mValidConnectType = new Type[validConnectionTypes.Length];
         validConnectionTypes.CopyTo(cr.mValidConnectType, 0);

         mConnectionRules.Add(cr);
      }
      public void clearConnectionRules()
      {
         mConnectionRules.Clear();
      }

      protected virtual bool isConnectionType(Type objType)
      {
         for(int i=0;i<mConnectionRules.Count;i++)
         {
            if (mConnectionRules[i].mTargetType == objType)
               return true;
         }
         return false;
      }

      int connectionTypeIndex(object objType)
      {
         for (int i = 0; i < mConnectionRules.Count; i++)
         {
            if (mConnectionRules[i].mTargetType == objType.GetType())
               return i;
         }
         return -1;
      }
      #endregion


      #region Compontents
      protected System.Windows.Forms.ContextMenuStrip contextMenuStrip1;
     
      protected virtual void InitializeComponent()
      {
         this.contextMenuStrip1 = new System.Windows.Forms.ContextMenuStrip();
        
         this.contextMenuStrip1.SuspendLayout();
         // 
         // contextMenuStrip1
         // 
         this.contextMenuStrip1.Name = "contextMenuStrip1";
         this.contextMenuStrip1.Size = new System.Drawing.Size(153, 48);
        
         

         this.contextMenuStrip1.ResumeLayout(false);
      }


      #endregion
   };

  
}