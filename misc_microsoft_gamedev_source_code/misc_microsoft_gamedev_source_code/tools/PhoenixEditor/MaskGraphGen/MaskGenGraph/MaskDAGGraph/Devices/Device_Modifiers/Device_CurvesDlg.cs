using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace graphapp
{
   public partial class Device_CurvesDlg : Form
   {
      public Device_Curves mOwningDevice = null;
      CurveControl mCurveControl = new CurveControl();
      public Device_CurvesDlg(Device_Curves owningDev)
      {
         mOwningDevice = owningDev;

         InitializeComponent();


         mCurveControl.Location = panel1.Location;
         mCurveControl.Size = panel1.Size;
         mCurveControl.ValueChanged += new EventHandler(mCurveControl_ValueChanged);
         panel1.Visible = false;
         this.Controls.Add(mCurveControl);

         populateFromParentList();
      }

      void apply()
      {
         mOwningDevice.clearControlPoints();
         for (int i = 0; i < mCurveControl.ControlPoints.Count; i++)
            mOwningDevice.addControlPoint(mCurveControl.ControlPoints.Keys[i] / 256.0f, mCurveControl.ControlPoints.Values[i] / 256.0f);
      }
      public void populateFromParentList()
      {
         mCurveControl.ControlPoints.Clear();
         for (int i = 0; i < mOwningDevice.ControlPointsKeys.Count; i++)
            mCurveControl.ControlPoints.Add((int)(mOwningDevice.ControlPointsKeys[i] * 256), (int)(mOwningDevice.ControlPointsValues[i] * 256));
      }
      void mCurveControl_ValueChanged(object sender, EventArgs e)
      {
         apply();
      }

      private void button2_Click(object sender, EventArgs e)
      {
         mCurveControl.ResetControlPoints();
         mOwningDevice.generatePreview();
      }

      private void button1_Click(object sender, EventArgs e)
      {
         apply();
         mOwningDevice.generatePreview();
      }


   }


   public class CurveControl
        : UserControl
   {
      private System.ComponentModel.Container components = null;
      private int[] curvesInvalidRange = new int[] { int.MaxValue, int.MinValue };
      private Point lastMouseXY = new Point(int.MinValue, int.MinValue);
      private int lastKey = -1;
      private int lastValue = -1;
      private bool tracking = false;
      private Point ptSave;
      private int pointsNearMousePerChannel;
      private bool effectChannel;


      #region init / Destroy
      protected internal CurveControl()
      {
         this.SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint |
             ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
         this.BorderStyle = BorderStyle.FixedSingle;


         // This call is required by the Windows.Forms Form Designer.
         InitializeComponent();

         ResetControlPoints();
        
      }

      /// <summary> 
      /// Clean up any resources being used.
      /// </summary>
      protected override void Dispose(bool disposing)
      {
         if (disposing)
         {
            if (components != null)
            {
               components.Dispose();
               components = null;
            }
         }
         base.Dispose(disposing);
      }

      /// <summary> 
      /// Required method for Designer support - do not modify 
      /// the contents of this method with the code editor.
      /// </summary>
      private void InitializeComponent()
      {
         this.TabStop = false;
      }
      #endregion

      protected int entries=256;
      public int Entries
      {
         get
         {
            return entries;
         }
      }
      protected SortedList<int, int> controlPoints;
      public SortedList<int, int> ControlPoints
      {
         get
         {
            return this.controlPoints;
         }

         set
         {
           
            this.controlPoints = value;
            Invalidate();
         }
      }


     
      public void ResetControlPoints()
      {
         controlPoints = new SortedList<int, int>();

        // for (int i = 0; i < Channels; ++i)
         {
            SortedList<int, int> newList = new SortedList<int, int>();

            controlPoints.Add(0, 0);
            controlPoints.Add(Entries - 1, Entries - 1);
         }

         Invalidate();
         OnValueChanged();
      }

      #region paint
      private void DrawToGraphics(Graphics g)
      {
         Color colorSolid = Color.FromArgb(this.ForeColor.A,this.ForeColor.R, this.ForeColor.G,this.ForeColor.B);
         Color colorGuide = Color.FromArgb(96, this.ForeColor.R, this.ForeColor.G, this.ForeColor.B);
         Color colorGrid = Color.FromArgb(128, this.ForeColor.R, this.ForeColor.G, this.ForeColor.B);

         Pen penSolid = new Pen(colorSolid, 1);
         Pen penGrid = new Pen(colorGrid, 1);
         Pen penGuide = new Pen(colorGuide, 1);

         penGrid.DashStyle = DashStyle.Dash;

         g.Clear(this.BackColor);
         g.SmoothingMode = SmoothingMode.AntiAlias;

         Rectangle ourRect = ClientRectangle;

         ourRect.Inflate(-1, -1);

         if (lastMouseXY.Y >= 0)
         {
            g.DrawLine(penGuide, 0, lastMouseXY.Y, Width, lastMouseXY.Y);
         }

         if (lastMouseXY.X >= 0)
         {
            g.DrawLine(penGuide, lastMouseXY.X, 0, lastMouseXY.X, Height);
         }

         //draw grid lines
         for (float f = 0.25f; f <= 0.75f; f += 0.25f)
         {
            float x = BMathLib.LERP(ourRect.Left, ourRect.Right, f);
            float y = BMathLib.LERP(ourRect.Top, ourRect.Bottom, f);

            g.DrawLine(penGrid,
                Point.Round(new PointF(x, ourRect.Top)),
                Point.Round(new PointF(x, ourRect.Bottom)));

            g.DrawLine(penGrid,
                Point.Round(new PointF(ourRect.Left, y)),
                Point.Round(new PointF(ourRect.Right, y)));
         }

         //draw 'perfect' line
         g.DrawLine(penGrid, ourRect.Left, ourRect.Bottom, ourRect.Right, ourRect.Top);



         float width = this.ClientRectangle.Width;
         float height = this.ClientRectangle.Height;

         {
            SortedList<int, int> channelControlPoints = controlPoints;
            int points = channelControlPoints.Count;

            Color color = Color.FromArgb(128,0,0,0);

           
            float penWidthSelected = 2;
            float penWidth = penWidthSelected;
            Pen penSelected = new Pen(color, penWidth);



            Pen pen = new Pen(color, penWidth);
            Brush brush = new SolidBrush(color);
            SolidBrush brushSelected = new SolidBrush(Color.White);

            SplineInterpolator interpolator = new SplineInterpolator();
            IList<int> xa = channelControlPoints.Keys;
            IList<int> ya = channelControlPoints.Values;
            PointF[] line = new PointF[Entries];

            for (int i = 0; i < points; ++i)
            {
               interpolator.Add(xa[i], ya[i]);
            }

            for (int i = 0; i < line.Length; ++i)
            {
               line[i].X = (float)i * (width - 1) / (entries - 1);
               line[i].Y = (float)(BMathLib.Clamp((float)(entries - 1 - interpolator.Interpolate(i)), 0, entries - 1)) *
                   (height - 1) / (entries - 1);
            }

            pen.LineJoin = LineJoin.Round;
            g.DrawLines(pen, line);

            //draw our control points
            for (int i = 0; i < points; ++i)
            {
               int k = channelControlPoints.Keys[i];
               float x = k * (width - 1) / (entries - 1);
               float y = (entries - 1 - channelControlPoints.Values[i]) * (height - 1) / (entries - 1);

               float radiusSelected =  8;
               float radiusNotSelected =  6;

               bool selected = (pointsNearMousePerChannel == i);
               float size = selected ? radiusSelected : (radiusNotSelected);
               RectangleF rect = new RectangleF(x - (size * 0.5f), y - (size * 0.5f), size, size);

               g.FillEllipse(selected ? brushSelected : brush, rect.X, rect.Y, rect.Width, rect.Height);
               g.DrawEllipse(selected ? penSelected : pen, rect.X, rect.Y, rect.Width, rect.Height);
            }

            pen.Dispose();
         }

         penSolid.Dispose();
         penGrid.Dispose();
         penGuide.Dispose();
      }

      protected override void OnPaint(PaintEventArgs e)
      {
         DrawToGraphics(e.Graphics);
         base.OnPaint(e);
      }
      #endregion

      #region on mouse

      public event EventHandler ValueChanged;
      protected virtual void OnValueChanged()
      {
         if (ValueChanged != null)
         {
            ValueChanged(this, EventArgs.Empty);
         }
      }

      //public event EventHandler<Point> CoordinatesChanged;
      protected virtual void OnCoordinatesChanged()
      {
         //   if (CoordinatesChanged != null)
         {
            //     CoordinatesChanged(this, new EventArgs<Point>(new Point(lastKey, lastValue)));
         }
      }

      protected override void OnMouseDown(MouseEventArgs e)
      {
         base.OnMouseDown(e);

         float width = this.ClientRectangle.Width;
         float height = this.ClientRectangle.Height;
         int mx = (int)BMathLib.Clamp(0.5f + e.X * (entries - 1) / (width - 1), 0, Entries - 1);
         int my = (int)BMathLib.Clamp(0.5f + Entries - 1 - e.Y * (entries - 1) / (height - 1), 0, Entries - 1);

       
         
            ptSave.X = -1;
         

         if (0 != e.Button)
         {
            tracking = (e.Button == MouseButtons.Left);
            lastKey = mx;

            bool anyNearMouse = false;

  
            {
               SortedList<int, int> channelControlPoints = controlPoints;
               int index = pointsNearMousePerChannel;
               bool hasPoint = (index >= 0);
               int key = hasPoint ? channelControlPoints.Keys[index] : index;

               anyNearMouse = (anyNearMouse || hasPoint);

               effectChannel = hasPoint;

               if ( hasPoint &&
                   key > 0 && key < entries - 1)
               {
                  channelControlPoints.RemoveAt(index);
                  OnValueChanged();
               }
            }

            if (!anyNearMouse)
            {
               for (int c = 0; c < 1; ++c)
               {
                  effectChannel = true;
               }
            }
         }

         OnMouseMove(e);
      }

      protected override void OnMouseUp(MouseEventArgs e)
      {
         base.OnMouseUp(e);

         if (0 != (e.Button & MouseButtons.Left) && tracking)
         {
            tracking = false;
            lastKey = -1;
         }
      }

      protected override void OnMouseMove(MouseEventArgs e)
      {
         base.OnMouseMove(e);

         lastMouseXY = new Point(e.X, e.Y);
         float width = this.ClientRectangle.Width;
         float height = this.ClientRectangle.Height;
         int mx = (int)BMathLib.Clamp(0.5f + e.X * (entries - 1) / (width - 1), 0, Entries - 1);
         int my = (int)BMathLib.Clamp(0.5f + Entries - 1 - e.Y * (entries - 1) / (height - 1), 0, Entries - 1);

         Invalidate();

         if (tracking && e.Button == MouseButtons.None)
         {
            tracking = false;
         }

         if (tracking)
         {
            bool changed = false;
            
            {
               SortedList<int, int> channelControlPoints = controlPoints;

               pointsNearMousePerChannel = -1;
               if (effectChannel)
               {
                  int lastIndex = channelControlPoints.IndexOfKey(lastKey);

                  if (ptSave.X >= 0 && ptSave.X != mx)
                  {
                     channelControlPoints[ptSave.X] = ptSave.Y;
                     ptSave.X = -1;

                     changed = true;
                  }
                  else if (lastKey > 0 && lastKey < Entries - 1 && lastIndex >= 0 && mx != lastKey)
                  {
                     channelControlPoints.RemoveAt(lastIndex);
                  }

                  if (mx >= 0 && mx < Entries)
                  {
                     int newValue = (int)BMathLib.Clamp((float)my, 0, (float)(Entries - 1));
                     int oldIndex = channelControlPoints.IndexOfKey(mx);
                     int oldValue = (oldIndex >= 0) ? channelControlPoints.Values[oldIndex] : -1;

                     if (oldIndex >= 0 && mx != lastKey)
                     {
                        // if we drag onto an existing point, delete it, but save it in case we drag away
                        ptSave.X = mx;
                        ptSave.Y = channelControlPoints.Values[oldIndex];
                     }

                     if (oldIndex < 0 ||
                         channelControlPoints[mx] != newValue)
                     {
                        channelControlPoints[mx] = newValue;
                        changed = true;
                     }

                     pointsNearMousePerChannel = channelControlPoints.IndexOfKey(mx);
                  }
               }
            }

            if (changed)
            {
               Update();
               OnValueChanged();
            }
         }
         else
         {
            
            {
               SortedList<int, int> channelControlPoints = controlPoints;
               int minRadiusSq = 30;
               int bestIndex = -1;

              // if (mask[c])
               {
                  for (int i = 0; i < channelControlPoints.Count; ++i)
                  {
                     int sumsq = 0;
                     int diff = 0;

                     diff = channelControlPoints.Keys[i] - mx;
                     sumsq += diff * diff;

                     diff = channelControlPoints.Values[i] - my;
                     sumsq += diff * diff;

                     if (sumsq < minRadiusSq)
                     {
                        minRadiusSq = sumsq;
                        bestIndex = i;
                     }
                  }
               }

               pointsNearMousePerChannel = bestIndex;
            }

            Update();
         }

         lastKey = mx;
         lastValue = my;
         OnCoordinatesChanged();
      }

      protected override void OnMouseLeave(EventArgs e)
      {
         lastKey = -1;
         lastValue = -1;
         lastMouseXY = new Point(int.MinValue, int.MinValue);
         Invalidate();
         OnCoordinatesChanged();
         base.OnMouseLeave(e);
      }
      #endregion
   }

   public sealed class SplineInterpolator
   {
      private SortedList<double, double> points = new SortedList<double, double>();
      private double[] y2;

      public int Count
      {
         get
         {
            return this.points.Count;
         }
      }

      public void Add(double x, double y)
      {
         points[x] = y;
         this.y2 = null;
      }

      public void Clear()
      {
         this.points.Clear();
      }

      // Interpolate() and PreCompute() are adapted from:
      // NUMERICAL RECIPES IN C: THE ART OF SCIENTIFIC COMPUTING
      // ISBN 0-521-43108-5, page 113, section 3.3.

      public double Interpolate(double x)
      {
         if (y2 == null)
         {
            PreCompute();
         }

         IList<double> xa = this.points.Keys;
         IList<double> ya = this.points.Values;

         int n = ya.Count;
         int klo = 0;     // We will find the right place in the table by means of
         int khi = n - 1; // bisection. This is optimal if sequential calls to this

         while (khi - klo > 1)
         {
            // routine are at random values of x. If sequential calls
            int k = (khi + klo) >> 1;// are in order, and closely spaced, one would do better

            if (xa[k] > x)
            {
               khi = k; // to store previous values of klo and khi and test if
            }
            else
            {
               klo = k;
            }
         }

         double h = xa[khi] - xa[klo];
         double a = (xa[khi] - x) / h;
         double b = (x - xa[klo]) / h;

         // Cubic spline polynomial is now evaluated.
         return a * ya[klo] + b * ya[khi] +
             ((a * a * a - a) * y2[klo] + (b * b * b - b) * y2[khi]) * (h * h) / 6.0;
      }

      private void PreCompute()
      {
         int n = points.Count;
         double[] u = new double[n];
         IList<double> xa = points.Keys;
         IList<double> ya = points.Values;

         this.y2 = new double[n];

         u[0] = 0;
         this.y2[0] = 0;

         for (int i = 1; i < n - 1; ++i)
         {
            // This is the decomposition loop of the tridiagonal algorithm. 
            // y2 and u are used for temporary storage of the decomposed factors.
            double wx = xa[i + 1] - xa[i - 1];
            double sig = (xa[i] - xa[i - 1]) / wx;
            double p = sig * y2[i - 1] + 2.0;

            this.y2[i] = (sig - 1.0) / p;

            double ddydx =
                (ya[i + 1] - ya[i]) / (xa[i + 1] - xa[i]) -
                (ya[i] - ya[i - 1]) / (xa[i] - xa[i - 1]);

            u[i] = (6.0 * ddydx / wx - sig * u[i - 1]) / p;
         }

         this.y2[n - 1] = 0;

         // This is the backsubstitution loop of the tridiagonal algorithm
         for (int i = n - 2; i >= 0; --i)
         {
            this.y2[i] = this.y2[i] * this.y2[i + 1] + u[i];
         }
      }
   }
}