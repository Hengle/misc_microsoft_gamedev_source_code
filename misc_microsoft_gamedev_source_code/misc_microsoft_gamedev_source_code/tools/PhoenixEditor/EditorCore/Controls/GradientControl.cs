using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Drawing.Drawing2D;

namespace EditorCore
{
   public partial class GradientControl : UserControl
   {
      public GradientControl()
      {
         InitializeComponent();

         //mPointList = new List<GradientPoint>();
         //clearPoints();
         //addPoint(0.0f, Color.Black);
         //addPoint(1.0f, Color.White);
         ResizeRedraw = true;
      }

      public void setData(List<GradientPoint> e)
      {
         mPointList = e;
         bInitialized = true;
         
      }      

      private List<GradientPoint> mPointList;
      private bool bInitialized = false;

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public List<GradientPoint> Gradient
      {
         set { this.mPointList = value; }
         get { return this.mPointList; }
      }

      public void insertPoint(int atIndex, double alpha, Color color)
      {
         if (!bInitialized)
            return;

         GradientPoint point = new GradientPoint();
         point.Color = color;
         point.Alpha = alpha;

         mPointList.Insert(atIndex, point);
      }

      public void addPoint(Color color)
      {
         if (!bInitialized)
            return;

         GradientPoint point = new GradientPoint();
         point.Alpha = 0.0f;
         point.Color = color;
         mPointList.Add(point);

         fixupAlphaValues();
      }

      public void addPoint(double alpha, Color color)
      {
         if (!bInitialized)
            return;

         if (alpha > 1.0f)
            alpha = 1.0f;

         if (alpha < 0.0f)
            alpha = 0.0f;

         GradientPoint point = new GradientPoint();
         point.Alpha = alpha;
         point.Color = color;

         mPointList.Add(point);
         Invalidate();
      }

      public void deletePoint(int index)
      {
         if (!bInitialized)
            return;

         if (index < 0 || index >= mPointList.Count)
            return;

         int oldCount = mPointList.Count;
         mPointList.RemoveAt(index);

         if (index == 0)
            mPointList[0].Alpha = 0.0f;

         else if (index == oldCount - 1)
            mPointList[mPointList.Count - 1].Alpha = 1.0f;
      }

      public void clearPoints()
      {
         mPointList.Clear();
      }

      private void fixupAlphaValues()
      {
         //-- fixup the alpha values
         for (int i = 0; i < mPointList.Count; i++)
         {
            GradientPoint p = mPointList[i];
            int numSegments = mPointList.Count - 1;
            p.Alpha = ((double)(i) / (double)numSegments);
         }
      }

      private void drawGradientRegion(Color color0, Color color1, Rectangle rect, PaintEventArgs e)
      {
         LinearGradientBrush brush = new LinearGradientBrush(rect, color0, color1, LinearGradientMode.Horizontal);
         e.Graphics.FillRectangle(brush, rect.X, rect.Y, rect.Width, rect.Height);
         brush.Dispose();
      }

      private void drawMarker(int x, PaintEventArgs e)
      {
         Pen pen = new Pen(Color.Black, 2);
         pen.Alignment = PenAlignment.Outset;
         pen.DashStyle = DashStyle.Dash;
         pen.LineJoin = LineJoin.Round;
         e.Graphics.DrawLine(pen, x, e.ClipRectangle.Y, x, e.ClipRectangle.Y + e.ClipRectangle.Height);
         


         Point[] points = new Point[3];
         int xOffset = 10;
         int yOffset = 7;
         points[0].X = x;
         points[0].Y = e.ClipRectangle.Y;
         points[1].X = x - xOffset;
         points[1].Y = e.ClipRectangle.Y + yOffset;
         points[2].X = x + xOffset;
         points[2].Y = e.ClipRectangle.Y + yOffset;

         pen.DashStyle = DashStyle.Solid;
         pen.LineJoin = LineJoin.Miter;
         e.Graphics.FillPolygon(Brushes.White, points);
         e.Graphics.DrawPolygon(pen, points);
         
         points[0].Y = e.ClipRectangle.Y + e.ClipRectangle.Height;
         points[1].Y = points[0].Y - yOffset;
         points[2].Y = points[0].Y - yOffset;

         e.Graphics.FillPolygon(Brushes.White, points);
         e.Graphics.DrawPolygon(pen, points);

         pen.Dispose();
      }

      private void GradientControl_Paint(object sender, PaintEventArgs e)
      {
         if (mPointList == null)
            return;

         //-- this should only happen in design view       
         if (bInitialized && mPointList.Count < 2)
         {
            clearPoints();
            addPoint(0.0f, Color.Black);
            addPoint(1.0f, Color.White);
            Invalidate();
         }         

         Rectangle rect = new Rectangle(e.ClipRectangle.X, e.ClipRectangle.Y, e.ClipRectangle.Width, e.ClipRectangle.Height);
         if (e.ClipRectangle.Width == 0)
            return;

         for (int i = 0; i < mPointList.Count - 1; i++)
         {
            GradientPoint p0 = mPointList[i];
            GradientPoint p1 = mPointList[i + 1];

            int x0 = (int) System.Math.Round(p0.Alpha * (double)e.ClipRectangle.Width, MidpointRounding.AwayFromZero);
            int x1 = (int) System.Math.Round(p1.Alpha * (double)e.ClipRectangle.Width, MidpointRounding.AwayFromZero);

            rect.X = x0;
            rect.Width = x1 - x0;
            if (rect.Width <= 0)
               continue;

            drawGradientRegion(p0.Color, p1.Color, rect, e);
            if (i != 0 && i != mPointList.Count -1)
               drawMarker(x0, e);
         }
      }

      
   }

   
}
