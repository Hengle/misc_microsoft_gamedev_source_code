using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class PreviewNavWindow : UserControl
   {
      public PreviewNavWindow()
      {
         InitializeComponent();

         this.AllowDrop = true;

         this.MouseMove += new MouseEventHandler(PreviewNavWindow_MouseMove);
         this.MouseDown += new MouseEventHandler(PreviewNavWindow_MouseDown);

         this.DragEnter += new DragEventHandler(PreviewNavWindow_DragEnter);
         this.DragDrop += new DragEventHandler(PreviewNavWindow_DragDrop);

         this.Load += new EventHandler(PreviewNavWindow_Load);
      }

      void PreviewNavWindow_Load(object sender, EventArgs e)
      {
         //WTF !!!!!!!! why does this break????????
         //UpdateViewLocation(new Point(0, 0));
      }



      void PreviewNavWindow_DragDrop(object sender, DragEventArgs e)
      {
         if ((mMasterControl == null) || mMasterControl.IsDisposed == true)
            return;         
         
         //object data = e.Data.GetData(typeof(UserControl));
         //UserControl otherButton = data as UserControl;
         string[] formats = e.Data.GetFormats();
         if (formats.Length == 0)
            return;
         UserControl otherButton = e.Data.GetData(e.Data.GetFormats()[0]) as UserControl;


         if (otherButton != null)
         {           
            if (e.Effect == DragDropEffects.Move)
            {

               Size masterLogicalSize = mMasterControl.AutoScrollMinSize;
               float xscale = (float)this.Width / masterLogicalSize.Width;
               float yscale = (float)this.Height / masterLogicalSize.Height;
               Point hitpoint = this.PointToClient(new Point(e.X, e.Y));
               otherButton.Location = new Point((int)(hitpoint.X / xscale) + mMasterControl.AutoScrollPosition.X, (int)(hitpoint.Y / yscale) + mMasterControl.AutoScrollPosition.Y);

               this.Refresh();
               mMasterControl.Refresh();
            }
         }
      }
      void PreviewNavWindow_DragEnter(object sender, DragEventArgs e)
      {
         e.Effect = DragDropEffects.None;
         string[] formats = e.Data.GetFormats();
         if(formats.Length == 0)
            return;
         UserControl otherButton = e.Data.GetData(e.Data.GetFormats()[0]) as UserControl;

            if (otherButton != null)
            {
               e.Effect = DragDropEffects.Move;
            }

      }
      void PreviewNavWindow_MouseDown(object sender, MouseEventArgs e)
      {
         if (e.Button != MouseButtons.Left)
            return;

         UpdateViewLocation(new Point(e.X, e.Y));
      }
      bool mbAllowNavWindowScroll = false;

      void PreviewNavWindow_MouseMove(object sender, MouseEventArgs e)
      {
         
         

            if (e.Button != MouseButtons.Left)
               return;
            UpdateViewLocation(new Point(e.X, e.Y));
            //mMasterControl.AutoScroll = false;

         

         //if ((mMasterControl == null) || mMasterControl.IsDisposed == true)
         //   return;
         //Size masterLogicalSize = mMasterControl.AutoScrollMinSize;
         ////Point masteroffset = mMasterControl.AutoScrollOffset;

         //float xscale = (float)this.Width / masterLogicalSize.Width;
         //float yscale = (float)this.Height / masterLogicalSize.Height;

         //mMasterControl.AutoScrollPosition = new Point((int)(e.X / xscale) - mMasterControl.Width / 2, (int)(e.Y / yscale) - mMasterControl.Height / 2);

         ////foreach (Control c in mMasterControl.Controls)
         ////{
         ////   c.
         ////}

         ////this.Invalidate();
         //this.Refresh();
         //mMasterControl.Refresh();
      }
      private void UpdateViewLocation(Point clickLoc)
      {
         //if (mbAllowNavWindowScroll == false)
         //   return;
         if ((mMasterControl == null) || mMasterControl.IsDisposed == true)
            return;
         Size masterLogicalSize = mMasterControl.AutoScrollMinSize;
         //Point masteroffset = mMasterControl.AutoScrollOffset;

         float xscale = (float)this.Width / masterLogicalSize.Width;
         float yscale = (float)this.Height / masterLogicalSize.Height;

         //mMasterControl.Refresh();
         ((NodeHostControl)mMasterControl).AutoScrollPosition = new Point((int)(clickLoc.X / xscale) - mMasterControl.Width / 2, (int)(clickLoc.Y / yscale) - mMasterControl.Height / 2);

         //mMasterControl.AutoScroll = false;

         //foreach (Control c in mMasterControl.Controls)
         //{
         //   c.
         //}

         //this.Invalidate();


         //this.Refresh();
         //mMasterControl.Refresh();


         //mMasterControl.AutoScroll = true;
         //mMasterControl.Refresh();

      }

      Dictionary<UserControl, bool> mConnectedControls = new Dictionary<UserControl, bool>();

      UserControl mMasterControl = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public UserControl MasterControl
      {
         set
         {
            mMasterControl = value;
            if(value != null)
            {
               if(mConnectedControls.ContainsKey(mMasterControl) == false)
               {
                  //mMasterControl
                  mMasterControl.Click += new EventHandler(mMasterControl_Click);
                  mMasterControl.MouseCaptureChanged += new EventHandler(mMasterControl_MouseCaptureChanged);
                  mMasterControl.ControlAdded += new ControlEventHandler(mMasterControl_ControlAdded);
                  mConnectedControls[mMasterControl] = true;
               }
               this.Invalidate();
            }
         }
         get
         {
            return mMasterControl;
         }

      }

      void mMasterControl_ControlAdded(object sender, ControlEventArgs e)
      {
         this.Invalidate();

         e.Control.Move += new EventHandler(Control_Move);
         //throw new Exception("The method or operation is not implemented.");
      }

      void Control_Move(object sender, EventArgs e)
      {
         this.Invalidate();
         //throw new Exception("The method or operation is not implemented.");
      }

      void mMasterControl_MouseCaptureChanged(object sender, EventArgs e)
      {
         //throw new Exception("The method or operation is not implemented.");
         this.Invalidate();
      }

      void mMasterControl_Click(object sender, EventArgs e)
      {
         //throw new Exception("The method or operation is not implemented.");
         this.Invalidate();
      }
      Pen mBlackPen = new Pen(Color.Black);
      Pen mBluePen = new Pen(Color.Blue);
      protected override void OnPaint(PaintEventArgs e)
      {
         if ((mMasterControl == null) || mMasterControl.IsDisposed == true) 
            return;
         Size masterLogicalSize = mMasterControl.AutoScrollMinSize;
         if (masterLogicalSize.Width == 0
         || masterLogicalSize.Height == 0)
            return;
         float xscale = (float)this.Width / masterLogicalSize.Width;
         float yscale = (float)this.Height / masterLogicalSize.Height ;

  
         foreach(Control c in mMasterControl.Controls)
         {
            int top = c.Top + -mMasterControl.AutoScrollPosition.Y;
            int left = c.Left + -mMasterControl.AutoScrollPosition.X;

            
            e.Graphics.DrawRectangle(mBlackPen, left * xscale, top * yscale, c.Width * xscale, c.Height * yscale);
         }
         Point masteroffset = mMasterControl.AutoScrollPosition;
         e.Graphics.DrawRectangle(mBluePen, -masteroffset.X * xscale, -masteroffset.Y * yscale, mMasterControl.Width * xscale, mMasterControl.Height * yscale);

         //base.OnPaint(e);
      }
   }
}
