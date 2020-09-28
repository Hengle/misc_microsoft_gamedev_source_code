using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace graphapp
{
   public partial class MaskGenForm : UserControl
   {
      DAGCanvas canvas = null;

      public MaskGenForm()
      {
         //this.DoubleBuffered = true;
         this.SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint |
                ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
         this.MouseWheel += new MouseEventHandler(Form1_MouseWheel);


         InitializeComponent();

         this.TabStop = false;
         
         
        
      }
      public void init()
      {
         canvas = new DAGCanvas(this.Width, this.Height);
      }
      public void setUpdateCallback(DAGCanvas.onUpdateCalculate callback)
      {
         canvas.onUpdateCallback += callback;
      }

      public DAGMask execute(int width, int height)
      {
         return canvas.execute(width, height);
      }
      #region saveload
      public void newCanvas()
      {
         canvas.cleanCanvas();
         canvas.newCanvas();
         savedFileName = "";
      }
      string savedFileName = "";
      public void saveCanvasToDisk()
      {
         if (savedFileName == "")
            saveCanvasAsToDisk();
         else
            canvas.saveCanvasToDisk(savedFileName);
      }
      public void saveCanvasAsToDisk()
      {
         SaveFileDialog sfd = new SaveFileDialog();
         sfd.Filter = "Mask Gen Script(*.mgs)|*.mgs";
         if (sfd.ShowDialog() == DialogResult.OK)
         {
            canvas.saveCanvasToDisk(sfd.FileName);
            savedFileName = sfd.FileName;
         }
      }
      public void loadCanvasFromDisk()
      {
         OpenFileDialog ofd = new OpenFileDialog();
         ofd.Filter = "Mask Gen Script(*.mgs)|*.mgs";
         if (ofd.ShowDialog() == DialogResult.OK)
            canvas.loadCanvasFromDisk(ofd.FileName);
      }

      public bool saveCanvasToMemoryStream(MemoryStream ms)
      {
         return canvas.saveCanvasToMemoryStream(ms);
      }

      public bool loadCanvasFromMemoryStream(MemoryStream ms)
      {
         return canvas.loadCanvasFromMemoryStream(ms);
      }

      #endregion
       
      protected override void OnPaint(PaintEventArgs e)
      {
         canvas.render(e.Graphics);
      }

      void Form1_MouseWheel(object sender, MouseEventArgs e)
      {
         canvas.onMouseScroll(e);
         Invalidate();
      }

      private void MaskGenForm_MouseDown(object sender, MouseEventArgs e)
      {
         canvas.onMouseDown(this.PointToScreen(e.Location), e);
         Invalidate();
      }

      private void MaskGenForm_MouseMove(object sender, MouseEventArgs e)
      {
         canvas.onMouseMove(this.PointToScreen(e.Location), e);
         Invalidate();
      }

      private void MaskGenForm_MouseUp(object sender, MouseEventArgs e)
      {
         canvas.onMouseUp(this.PointToScreen(e.Location), e);
         Invalidate();
      }

      private void MaskGenForm_KeyDown(object sender, KeyEventArgs e)
      {
         canvas.onKeyDown(e);
         Invalidate();
      }

      private void MaskGenForm_KeyUp(object sender, KeyEventArgs e)
      {
         canvas.onKeyUp(e);
         Invalidate();
      }

      

      private void MaskGenForm_MouseDoubleClick(object sender, MouseEventArgs e)
      {
         canvas.onDoubleClick(this.PointToScreen(e.Location), e);
         Invalidate();
      }


   }
}
