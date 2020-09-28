using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Drawing;
using System.ComponentModel;


namespace PhoenixEditor.ScenarioEditor
{

   public class NodeHostControl : UserControl, IObstructionProvider
   {
      public NodeHostControl()
      {
         this.MouseDown += new MouseEventHandler(NodeHostControl_MouseDown);
         this.MouseMove += new MouseEventHandler(NodeHostControl_MouseMove);
         this.MouseUp += new MouseEventHandler(NodeHostControl_MouseUp);
         this.KeyDown += new KeyEventHandler(NodeHostControl_KeyDown);

         this.Paint += new PaintEventHandler(NodeHostControl_Paint);

         this.AllowDrop = true;
         this.DoubleBuffered = true;

         this.AutoScroll = true;
         this.HScroll = false;
         this.VScroll = false;


         //this.AutoScroll = true;//test
         //this.HScroll = false;
         //this.VScroll = false;

         //SetStyle(ControlStyles.Opaque, true);
         //this.MaximumSize = new Size(3000, 3000);
         //SetStyle(ControlStyles.OptimizedDoubleBuffer, true);
         //this.DoubleBuffered = false;
         //this.BufferContext
         //BufferedGraphicsManager.Current.MaximumBuffer = new Size(6000, 6000);

         //BufferedGraphicsManager.Current.Dispose();
         //BufferedGraphicsManager.Current = new BufferedGraphicsManager();
         //BufferedGraphicsManager.Current.Allocate(this.CreateGraphics(), new Rectangle(0, 0, 6000, 6000));
      }
      
      public int mGroupID = -1;

      INodeHostControlOwner mOwner = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public INodeHostControlOwner Owner
      {
         set
         {
            mOwner = value;
         }
         get
         {
            return mOwner;
         }
      }

      void NodeHostControl_KeyDown(object sender, KeyEventArgs e)
      {
         if (e.KeyCode == Keys.Delete)
         {
            DeleteSelected(false);
         }
      }

      public void SetDirty()
      {
         this.Refresh();
      }

      bool mbDirtyNextEvent = false;
      public void QueueDirty()
      {
         mbDirtyNextEvent = true;
      }

      protected override void OnMouseMove(MouseEventArgs e)
      {
         if (mbDirtyNextEvent == true)
         {
            SetDirty();
            mbDirtyNextEvent = false;
         }
         base.OnMouseMove(e);
      }

      public List<Control> GetSelected()
      {
         List<Control> selected = new List<Control>();
         foreach (Control c in this.Controls)
         {
            ISelectable s = c as ISelectable;
            if (s != null && s.IsSelected())
            {
               selected.Add(c);
            }
         }
         return selected;
      }


      public void MoveSelected(int x, int y, int keyPosX, int keyPosY)
      {
         foreach (Control c in this.Controls)
         {
            ISelectable s = c as ISelectable;
            if (s != null && s.IsSelected())
            {
               ClientNodeControl cn = c as ClientNodeControl;
               if (cn != null)
               {
                  cn.MoveControl(x, y, keyPosX, keyPosY);
               }
            }
         }
      }

      public void DeleteSelected(bool noWarning)
      {
         if (noWarning || MessageBox.Show("Delete", "Delete?", MessageBoxButtons.OKCancel) == DialogResult.OK)
         {
            List<IDeletable> hitlist = new List<IDeletable>();
            foreach (Control c in this.Controls)
            {
               ISelectable selectable = c as ISelectable;
               if (selectable != null)
               {
                  if (selectable.IsSelected())
                  {
                     IDeletable deleteme = selectable as IDeletable;
                     if (deleteme != null)
                     {
                        hitlist.Add(deleteme);
                     }
                  }
               }
            }
            foreach (IDeletable item in hitlist)
            {
               item.Delete();
            }
         }
      }

      public Point mLastPosition = Point.Empty;

      private Point mLastMouseMovePos = new Point(-1, -1);
      private Point mLastAutoScrollPosition;

      void NodeHostControl_MouseUp(object sender, MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Middle)
         {
            Cursor.Current = Cursors.Arrow;
         }

         //mLastPosition = Point(e.X, e.Y);
         if (e.Button == MouseButtons.Right)
         {
            if(mOwner != null)
               mOwner.ShowContextMenu(this, e.X, e.Y);
         }
         if (e.Button == MouseButtons.Left)
         {
            int top = Math.Min(mLastPosition.Y, e.Y);
            int bottom = Math.Max(mLastPosition.Y, e.Y);
            int left = Math.Min(mLastPosition.X, e.X);
            int right = Math.Max(mLastPosition.X, e.X);
            Rectangle r = Rectangle.FromLTRB(left, top, right, bottom);

            List<Control> selected = new List<Control>();
            foreach (Control c in this.Controls)
            {
               ISelectable selectable = c as ISelectable;
               if (selectable != null)
               {
                  Rectangle bounds = selectable.GetBounds();
                  Rectangle intersect = Rectangle.Intersect(bounds, r);
                  if (intersect != Rectangle.Empty)
                  {
                     Rectangle union = Rectangle.Union(bounds, r);
                     if (union != null)
                     {
                        selected.Add(c);
                        selectable.SelectControl();
                     }
                     else
                     {
                        selectable.DeSelectControl();
                     }
                  }
                  else
                  {
                     //Keyboard.
                     //Keys.
                     if(Control.ModifierKeys != Keys.Shift)
                        selectable.DeSelectControl();
                  }
               }
            }

            {
               r.Offset(this.PointToScreen(new Point(0, 0)));

               ControlPaint.DrawReversibleFrame(r, Color.White, FrameStyle.Thick);
            }
         }
      }

      void NodeHostControl_MouseDown(object sender, MouseEventArgs e)
      {
         mLastPosition = new Point(e.X, e.Y);
         mLastMouseMovePos = mLastPosition;

         mLastAutoScrollPosition = AutoScrollPosition;

         if (e.Button == MouseButtons.Middle)
         {
            Cursor.Current = Cursors.SizeAll;
         }
      }
      int scrollThreashold = 100;
      Point mLastScrollPosition = new Point(0,0);
      void NodeHostControl_MouseMove(object sender, MouseEventArgs e)
      {
         scrollThreashold = 25;
         if (e.Button == MouseButtons.Middle)
         {
            //BufferedGraphicsManager.Current.MaximumBuffer = new Size(3000, 3000);

            //AutoScroll = false;

            if (mLastMouseMovePos.X == -1)
               mLastMouseMovePos = new Point(e.X, e.Y);
            Point p = new Point(e.X, e.Y);

            int xdelta = mLastPosition.X - e.X;
            int ydelta = mLastPosition.Y - e.Y;


            int sx = mLastScrollPosition.X - e.X;
            int sy = mLastScrollPosition.Y - e.Y;
            if (Math.Sqrt(sx * sx + sy * sy) < scrollThreashold)
            {

               return;
            }
            else
            {
               mLastScrollPosition = new Point(e.X, e.Y);
            }

            AutoScrollPosition = new Point(-(mLastAutoScrollPosition.X - xdelta), -(mLastAutoScrollPosition.Y - ydelta));

            //AutoScroll = false;

            //this.Refresh();

            mLastMouseMovePos = p;
         }

         if (e.Button == MouseButtons.Left)
         {
            if (mLastMouseMovePos.X == -1)
               mLastMouseMovePos = new Point(e.X, e.Y);

            Point p = new Point(e.X, e.Y);
            Graphics g = Graphics.FromHwnd(this.Handle);

            {
               int top = Math.Min(mLastPosition.Y, mLastMouseMovePos.Y);
               int bottom = Math.Max(mLastPosition.Y, mLastMouseMovePos.Y);
               int left = Math.Min(mLastPosition.X, mLastMouseMovePos.X);
               int right = Math.Max(mLastPosition.X, mLastMouseMovePos.X);
               Rectangle r = Rectangle.FromLTRB(left, top, right, bottom);
               r.Offset(this.PointToScreen(new Point(0, 0)));
               ControlPaint.DrawReversibleFrame(r, Color.White, FrameStyle.Thick);
            }
            {
               int top = Math.Min(mLastPosition.Y, e.Y);
               int bottom = Math.Max(mLastPosition.Y, e.Y);
               int left = Math.Min(mLastPosition.X, e.X);
               int right = Math.Max(mLastPosition.X, e.X);
               Rectangle r = Rectangle.FromLTRB(left, top, right, bottom);
               r.Offset(this.PointToScreen(new Point(0, 0)));

               ControlPaint.DrawReversibleFrame(r, Color.Black, FrameStyle.Thick);
            }
            mLastMouseMovePos = p;
         }
      }


      public int GetCPLevel(IControlPoint cp)
      {
         ClientNodeControl node = cp.TagObject as ClientNodeControl;

         return node.GetGroupID();
      }

      //protected override void OnPaint(PaintEventArgs e)
      //{
      //   base.OnPaint(e);
      //}
      //protected override void OnPaintBackground(PaintEventArgs e)
      //{
      //   //e.Graphics.setc
      //   //base.OnPaintBackground(e);
      //}

      Pen mTriggerPen = new Pen(Color.LightSkyBlue, 2);
      Pen mTriggerActivatePen = new Pen(Color.LightGreen, 2);
      Pen mTriggerDeactivatePen = new Pen(Color.Red, 2);
      Pen mHighlightPen = new Pen(Color.Gold, 3);
      Point last = Point.Empty;
      void NodeHostControl_Paint(object sender, PaintEventArgs e)
      {
         foreach (Control c in this.Controls)
         {
            IControlPointOwner cpowner = c as IControlPointOwner;
            if (cpowner != null)
            {
               List<KeyValuePair<IControlPoint, IControlPoint>> hitlist = new List<KeyValuePair<IControlPoint, IControlPoint>>();
               foreach (IControlPoint cp in cpowner.GetControlPoints())
               {
                  //if (cp.isConnected())
                  {
                     foreach (IControlPoint target in cp.GetTargets())
                     {
                        if (target.MarkForDelete == true || cp.MarkForDelete == true)
                        {
                           //cp.DisconnectControlPoint(target);
                           hitlist.Add(new KeyValuePair<IControlPoint, IControlPoint>(cp, target));
                           continue;
                        }


                        if (target == null)
                           continue;
                        IControlPoint realTarget = target;
                        //proxy method
                        //if (mOwner != null)
                        //{
                        //   realTarget = mOwner.GetProxyControlPoint(this, target);
                        //}

                        //show @ level method:
                        if (GetCPLevel(target) != this.mGroupID)
                        {
                           continue;
                        }

                        //TODO: add the concept endpoints having a color

                        Pen p = mTriggerPen;
                        if (realTarget.ToString() == "Activate")
                        {
                           p = mTriggerActivatePen;
                        }
                        if (realTarget.ToString() == "Deactivate")
                        {
                           p = mTriggerDeactivatePen;
                        }

                        if (cp.HighLight == true || realTarget.HighLight == true)
                           p = mHighlightPen;

                        Point P1 = PointToClient(cp.GetPosition());
                        Point V1 = PointToClient(cp.GetVector());
                        Point V2 = PointToClient(realTarget.GetVector());
                        Point P2 = PointToClient(realTarget.GetPosition());


                        //smart(ish) routing for horizontal links...

                        //wire to self 
                        if (P2.Y == P1.Y && Math.Abs(P1.X - P2.X) == 300)
                        {
                           V1.Y -= 30;
                           V2.Y -= 30;
                           e.Graphics.DrawBezier(p,
                              P1,
                              (V1),
                              (V2),
                              P2);
                        }
                        else if (P2.X - P1.X < -50)// && P2.X - P1.X < 250)
                        {
                           //if (P2.X - P1.X > 0 && P2.X - P1.X < 250)
                           //{
                           //   int num = (250 * (250 - (P2.X - P1.X)) / 250);
                           //   V1.X -= num;
                           //   V2.X += num;
                           //}
                           //int num = (250 * (250 - (P2.Y - P1.Y)) / 250);
                           int num = (P2.Y - P1.Y) / 2;
                           if (V2.Y > V1.Y)
                           {
                              V1.Y += 2 * num;
                              V2.Y -= 30;
                           }
                           else
                           {
                              V1.Y -= 30;
                              V2.Y -= 2 * num;

                           }
                           e.Graphics.DrawBezier(p,
                              P1,
                              (V1),
                              (V2),
                              P2);
                        }
                        else
                        {
                           if (P2.X - P1.X > 0 && P2.X - P1.X < 250)
                           {
                              int num = (250 * (250 - (P2.X - P1.X)) / 250);
                              V1.X -= num;
                              V2.X += num;
                           }
                           else if (P2.X - P1.X > -50 && P2.X - P1.X < 0)
                           {
                              int num = (250 * (-50 - (P2.X - P1.X)) / 50);
                              V1.X += num;
                              V2.X -= num;
                           }

                           e.Graphics.DrawBezier(p,
                              P1,
                              (V1),
                              (V2),
                              P2);

                        }


                     }

                     //clean up



                  }
               }
               foreach (KeyValuePair<IControlPoint, IControlPoint> target in hitlist)
               {
                  target.Key.DisconnectControlPoint(target.Value);
               }
            }


         }

         //base.AutoScroll = true;
         //base.AutoScrollPosition = mAutoScrollPosition;
         //base.AutoScroll = false;

      }


      //void TriggerEditor_Paint(object sender, PaintEventArgs e)
      //{
      //   //throw new Exception("The method or operation is not implemented.");

      //   base.AutoScrollPosition = mAutoScrollPosition;
      //}

      bool bOnce = true;

      Point mAutoScrollPosition = new Point(0, 0);
      new public Point AutoScrollPosition
      {
         get
         {
            return base.AutoScrollPosition;// mAutoScrollPosition;
         }
         set
         {
            mAutoScrollPosition = value;

            this.SetDisplayRectLocation2(-value.X, -value.Y);
            //this.AutoScroll = true;
            //this.HScroll = false;
            //this.VScroll = false;
            //this.VerticalScroll.Visible = false;
            //this.HorizontalScroll.Visible = false;
            //this.AutoScroll = false;
            //this.SyncScrollbars(true); 
            //this.scrollPosition = value; 


            if (bOnce)
            {
               this.AutoScroll = true;
               base.AutoScrollPosition = mAutoScrollPosition;
               this.AutoScroll = false;
               bOnce = false;
            }
            else
            {
               base.AutoScrollPosition = mAutoScrollPosition;
            }


         //   //this.Invalidate();
         //   base.SuspendLayout();
         //   //base.AutoScroll = true;
         //   //base.AutoScroll = true;
         //   this.Padding = new Padding(1);
         //   this.SetDisplayRectLocation2(-value.X, -value.Y);
         //   //this.SetScrollState(1, true);
         //   //this.SetScrollState(8, true);
            

         //   //if (this.SetDisplayRectangleSize(this.MinimumSize.Width, this.MinimumSize.Height))
         //   {

         //      this.SetDisplayRectLocation(-value.X, -value.Y);
         //   }
         //   this.SetDisplayRectLocation(-value.X, -value.Y);

         //   //System.Windows.Forms.SafeNativeMethods.ScrollWindowEx(new HandleRef(this, base.Handle), num1, num2, null, ref rect1, NativeMethods.NullHandleRef, ref rect2, 7);

         //   //base.AutoScroll = false;
         //   //base.VScroll = false;
         //   //base.HScroll = false;
         //   base.ResumeLayout();
         }
      }

      protected void SetDisplayRectLocation2(int x, int y)
      {
         int num1 = 0;
         int num2 = 0;
         Rectangle rectangle1 = base.ClientRectangle;
         Rectangle rectangle2 = this.DisplayRectangle;
         //Rectangle rectangle3 = ((System.Windows.Forms.Layout.IArrangedElement)this).DisplayRectangle;

         int num3 = Math.Min(rectangle1.Width - rectangle2.Width, 0);
         int num4 = Math.Min(rectangle1.Height - rectangle2.Height, 0);
         if (x > 0)
         {
            x = 0;
         }
         if (y > 0)
         {
            y = 0;
         }
         if (x < num3)
         {
            x = num3;
         }
         if (y < num4)
         {
            y = num4;
         }
         if (rectangle2.X != x)
         {
            num1 = x - rectangle2.X;
         }
         if (rectangle2.Y != y)
         {
            num2 = y - rectangle2.Y;
         }
         //this.DisplayRectangle.X = x;
         //this.DisplayRectangle.Y = y;

         //if ((num1 != 0) || ((num2 != 0) && base.IsHandleCreated))
         //{
         //   Rectangle rectangle3 = base.ClientRectangle;
         //   NativeMethods.RECT rect1 = NativeMethods.RECT.FromXYWH(rectangle3.X, rectangle3.Y, rectangle3.Width, rectangle3.Height);
         //   NativeMethods.RECT rect2 = NativeMethods.RECT.FromXYWH(rectangle3.X, rectangle3.Y, rectangle3.Width, rectangle3.Height);
         //   SafeNativeMethods.ScrollWindowEx(new HandleRef(this, base.Handle), num1, num2, null, ref rect1, NativeMethods.NullHandleRef, ref rect2, 7);
         //}
         //for (int num5 = 0; num5 < base.Controls.Count; num5++)
         //{
         //   Control control1 = base.Controls[num5];
         //   if ((control1 != null) && control1.IsHandleCreated)
         //   {
         //      control1.UpdateBounds();
         //   }
         //}
      }

 



      public void DetachControl(Control c)
      {
         this.Controls.Remove(c);
      }


      Point mLastInsertPos = new Point(5, 5);
      public void InsertControl(Control c, int x, int y)
      {
         if (x != -1 || y != -1)
         {
            if (x < 0)
               x = 0;
            if (y < 0)
               y = 0;

            if (x > this.AutoScrollMinSize.Width)
               x = this.AutoScrollMinSize.Width - 300;
            if (y > this.AutoScrollMinSize.Height)
               y = this.AutoScrollMinSize.Height - 300;

            c.Location = new Point(x, y);

            
         }
         else
         {
            c.Location = mLastInsertPos;

            if (mLastInsertPos.X < 1200)
            {
               mLastInsertPos.X += (c.Width + 10);
            }
            else
            {
               mLastInsertPos.Y += 300;
               mLastInsertPos.X = 5;
            }
         }

         if(c is ClientNodeControl)
         {
            ((ClientNodeControl)c).SetGroupID(this.mGroupID);
         }

         this.Controls.Add(c);
         if (mOwner != null)
            mOwner.ControlAdded(c);
      }
      protected void ClearControls()
      {
         mLastInsertPos = new Point(5, 5);
         this.Controls.Clear();
      }

      public List<Rectangle> GetObstructions()
      {
         List<Rectangle> bounds = new List<Rectangle>();
         foreach (Control c in this.Controls)
         {
            ISelectable selectable = c as ISelectable;
            if (selectable != null)
            {
               bounds.Add(selectable.GetBounds());
            }
         }
         return bounds;
      }
   }

   #region Interfaces


   public interface IControlPointOwner
   {
      List<IControlPoint> GetControlPoints();

   }

   public interface INodeHostControlOwner
   {
      //IControlPoint GetProxyControlPoint(NodeHostControl host, IControlPoint cp);
      void ControlAdded(Control c);
      void ShowContextMenu(Control sender, int x, int y);
      void SetDirty();
   }


   public interface IControlPoint
   {
      bool isConnected();
      ICollection<IControlPoint> GetTargets();
      Point GetPosition();
      Point GetVector();

      void ConnectControlPoint(IControlPoint cpTarget);
      void DisconnectControlPoint(IControlPoint cpTarget);
      bool CanConnect(IControlPoint cpother);

      bool HighLight
      {
         get;
         set;
      }
      bool HasProxy
      {
         get;
         set;
      }
      bool Virtual
      {
         get;
         set;
      }
      IControlPoint ProxyTarget
      {
         get;
         set;
      }

      List<KeyValuePair<IControlPoint, IControlPoint>> ParentConnections
      {
         get;
         set;
      }

      string ControlPointDescription
      {
         get;
         set;
      }
      string TypeKey
      {
         set;
         get;
      }
      string TargetKey
      {
         set;
         get;
      }
      object TagObject
      {
         get;
         set;
      }

      IControlPoint ChainParent
      {
         get;
         set;
      }
      IControlPoint ChainChild
      {
         get;
         set;
      }

      bool MarkForDelete
      {
         get;
         set;
      }

      string GetName();

      int GetLevel();
   }

   public interface IObstructionProvider
   {
      List<Rectangle> GetObstructions();
   }

   public interface ISelectable
   {
      Rectangle GetBounds();
      void SelectControl();
      void DeSelectControl();
      bool IsSelected();

   }
   public interface IDeletable
   {
      void Delete();
   }

   public interface ICommentOutable
   {
      bool CommentOut
      {
         set;
         get;
      }

   }


   public interface IMoveable
   {
      void Move(int x, int y);
   }

   #endregion



   #region SomeLineStuff
   public class SmartConnection
   {
      Point mA;
      Point mB;
      Point mALast = Point.Empty;
      Point mBLast = Point.Empty;
      bool mbDirty = false;
      Control mOwner;
      IObstructionProvider mProvider;
      public SmartConnection(IObstructionProvider provider, Control Owner)
      {
         mOwner = Owner;
         mProvider = provider;

      }

      public Point A
      {
         get
         {
            return mA;
         }
         set
         {
            mALast = A;
            mA = value;
            if (mA == mALast)
            {
               mbDirty = true;
               //Paint();
            }
         }
      }
      public Point B
      {
         get
         {
            return mB;
         }
         set
         {
            mBLast = B;
            mB = value;
            if (mB == mBLast)
            {
               mbDirty = true;
               //Paint();
            }
         }
      }

      //private void ComputePath()
      //{
      //   List<Rectangle> rects = mProvider.GetObstructions();

      //   int xdir = mB.X - mA.X;
      //   int ydir = mB.Y - mA.Y;

      //   int xpos = mA.X;
      //   int ypos = mA.Y;

      //   bool bHorizontal = (xdir*xdir > ydir*ydir)? true: false;

      //   if(bHorizontal)
      //   {
      //      int intersection = -1;
      //      foreach(Rectangle r in rects)
      //      {
      //         if(r.Top < ypos && r.Bottom > ypos)
      //         {
      //            //if(xdir - r.Left  < )

      //         }
      //      }
      //   }

      //}



   }


   public class SmartLineSegment
   {
      Point mA;
      Point mB;
      Point mALast = Point.Empty;
      Point mBLast = Point.Empty;
      bool mbDirty = false;

      Control mOwner;
      Color mColor = Color.Red;
      public SmartLineSegment(Control Owner)
      {
         mOwner = Owner;
      }

      public Point A
      {
         get
         {
            return mA;
         }
         set
         {
            mALast = A;
            mA = value;
            if (mA == mALast)
            {
               mbDirty = true;
               Paint();
            }
         }
      }
      public Point B
      {
         get
         {
            return mB;
         }
         set
         {
            mBLast = B;
            mB = value;
            if (mB == mBLast)
            {
               mbDirty = true;
               Paint();
            }
         }
      }

      public void Paint()
      {
         if (mbDirty == true)
         {
            EraseLast();
            ControlPaint.DrawReversibleLine(
               (mA),
               (mB),
               mColor);
            //ControlPaint.DrawReversibleLine(
            //   mOwner.PointToScreen(mA),
            //   mOwner.PointToScreen(mB),
            //   mColor);
         }
         mbDirty = false;
      }

      public void EraseLast()
      {
         if (mBLast != Point.Empty)
         {
            ControlPaint.DrawReversibleLine(
               (mALast),
               (mBLast),
               mColor);
            //ControlPaint.DrawReversibleLine(
            //   mOwner.PointToScreen(mALast),
            //   mOwner.PointToScreen(mBLast),
            //   mColor);
         }
      }
   }

   #endregion


}
