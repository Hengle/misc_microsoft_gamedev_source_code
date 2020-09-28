using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ClientNodeControl : UserControl
   {
      public NodeHostControl mHost = null;
      public ClientNodeControl()
      {
         InitializeComponent();
       
      }

      List<Control> mDragControls = new List<Control>();
      protected void AddDragSurface(Control c)
      {
         mDragControls.Add(c);
         c.MouseMove += new MouseEventHandler(c_MouseMove);
         c.MouseDown += new MouseEventHandler(c_MouseDown);
         c.MouseUp += new MouseEventHandler(c_MouseUp);
      }
      List<Control> mMinMaxControls = new List<Control>();
      protected void AddMinMaxButton(Control minMax)
      {
         mMinMaxControls.Add(minMax);
         minMax.Click += new EventHandler(minMax_Click);

      }
      int mOldHeight = 30;
      void minMax_Click(object sender, EventArgs e)
      {
         if (Height < 35)
         {
            Height = mOldHeight;
         }
         else
         {
            mOldHeight = Height;
            Height = 30;
         }

      }
      List<Control> mResizeControls = new List<Control>();
      protected void AddResizeSurface(Control resizeSurface)
      {
         mResizeControls.Add(resizeSurface);
         resizeSurface.MouseMove += new MouseEventHandler(resizeSurface_MouseMove);
         resizeSurface.MouseDown += new MouseEventHandler(resizeSurface_MouseDown);
         resizeSurface.MouseUp += new MouseEventHandler(resizeSurface_MouseUp);
      }

      bool mbResizing = false;
      void resizeSurface_MouseUp(object sender, MouseEventArgs e)
      {
         //throw new Exception("The method or operation is not implemented.");
      }

      void resizeSurface_MouseDown(object sender, MouseEventArgs e)
      {
         //throw new Exception("The method or operation is not implemented.");
         mLastOffset = Point.Empty;

      }

      void resizeSurface_MouseMove(object sender, MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Left)
         {
            if (mLastOffset == Point.Empty)
            {

               mLastOffset = (new Point(e.X, e.Y));
               //StartMove();
               return;
            }
            else
            {       
               int xdelta = e.X - mLastOffset.X;
               int ydelta = e.Y - mLastOffset.Y;

               this.Width += xdelta;
               this.Height += ydelta;

               if (this.Width < 30) this.Width = 30;
               if (this.Height < 30) this.Height = 30;
            }

         }
      }






      void c_MouseUp(object sender, MouseEventArgs e)
      {
         if(this.mbMoving)
         {
            mHost.SetDirty();

            mbMoving = false;
         }
      }

      void c_MouseDown(object sender, MouseEventArgs e)
      {
         mLastOffset = Point.Empty;
      }
      virtual public void OnMoved()
      {

      }
      bool mbMoving = false;
      Point mLastOffset = Point.Empty;

      protected int mGroupID = -1;
      public virtual int GetGroupID(){return mGroupID;}
      public virtual void SetGroupID(int id) { mGroupID = id; }


      void c_MouseMove(object sender, MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Left)
         {
            if (mLastOffset == Point.Empty)
            {

               mLastOffset = (new Point(e.X, e.Y));
               //StartMove();
               return;
            }
            else
            {
               //Move(e.X,e.Y,mLastOffset.X, mLastOffset.Y);
               if (this is ISelectable && ((ISelectable)this).IsSelected())
               {

                  mHost.MoveSelected(e.X, e.Y, mLastOffset.X, mLastOffset.Y);
               }
               else
               {
                  MoveControl(e.X, e.Y, mLastOffset.X, mLastOffset.Y);
               }
            }

         }

      }

      void StartMove()
      {
         mLastOffset = new Point(this.Left, this.Top);

      }
      //void EndMove()
      //{
      //   mLastOffset = Point.e
      //}

      public void MoveControl(int x, int y, int keyPosX, int keyPosY)
      {      
         mbMoving = true;
         Point thisPosition = this.PointToScreen(new Point(x, y));
         thisPosition = this.Parent.PointToClient(thisPosition);
         thisPosition.Offset(-keyPosX, -keyPosY);


         bool hitTopBoundry = false;

         if (thisPosition.Y < 0)
         {
            hitTopBoundry = true;
         }

         if (thisPosition.X - mHost.AutoScrollPosition.X > 0 && thisPosition.Y - mHost.AutoScrollPosition.Y > 0)
         {
            this.Location = thisPosition;
            //OnMoved();
         }
         else if (thisPosition.Y - mHost.AutoScrollPosition.Y > 0) //slide
         {
            thisPosition.X = 0;
            this.Location = thisPosition;

            //OnMoved();
         }
         else if (thisPosition.X - mHost.AutoScrollPosition.X > 0) //slide
         {
            thisPosition.Y = 0;
            this.Location = thisPosition;
            //OnMoved();

         }

         if (thisPosition.X - mHost.AutoScrollPosition.X > mHost.AutoScrollMinSize.Width - 300)
         {
            thisPosition.X = mHost.AutoScrollMinSize.Width + mHost.AutoScrollPosition.X - 300;
            this.Location = thisPosition;
         }
         if (thisPosition.Y - mHost.AutoScrollPosition.Y > mHost.AutoScrollMinSize.Height - 300)
         {
            thisPosition.Y = mHost.AutoScrollMinSize.Height + mHost.AutoScrollPosition.Y - 300;
            this.Location = thisPosition;
         }
         OnMoved();

         //if (thisPosition != this.Location)
         //{
         //   OnMoved();
         //}

         if (hitTopBoundry && thisPosition.X < 200)
         {
            DragOutOfHost();
         }

         mHost.Invalidate(false);
         return;

         IControlPointOwner cpowner = this as IControlPointOwner;
         if (cpowner != null)
         {
            foreach (IControlPoint cp in cpowner.GetControlPoints())
            {
               if (cp.isConnected())
               {
                  foreach (IControlPoint target in cp.GetTargets())
                  {
                     if (target == null)
                        continue;

                     Point p1 = mHost.PointToClient(cp.GetPosition());
                     Point p2 = mHost.PointToClient(target.GetPosition());
                     int top = Math.Min(p1.Y, p2.Y);
                     int bottom = Math.Max(p1.Y, p2.Y);
                     int left = Math.Min(p1.X, p2.X);
                     int right = Math.Max(p1.X, p2.X);

                     Rectangle r = Rectangle.FromLTRB(left - 10, top - 10, right + 10, bottom + 10);
                     mHost.Invalidate(r, false);
                  }
               }
            }
         }

      



      }
   
      public void DragOutOfHost()
      {
         DoDragDrop(this, DragDropEffects.All);
      }


   }
   

}
