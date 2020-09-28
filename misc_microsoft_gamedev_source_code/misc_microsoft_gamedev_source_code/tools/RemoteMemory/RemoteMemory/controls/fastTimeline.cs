using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace RemoteMemory
{
   public partial class fastTimeline : UserControl
   {
      class GraphLine
      {
         public List<int> mYValues = new List<int>();
         public uint mHandle = 0;
         public string mName = "";
         public Color mColor = Color.Blue;
         public bool mEnabled = true;
      };

      List<GraphLine> mGraphLines= new List<GraphLine>();


      DateTime mStartTime;

      int mMaxYValue = 1;
      int mDrawDistanceBetweenYTicks = 1;

      int mMaxXValue = 1;
      int mDrawDistanceBetweenXTicks = 5;

      float mTranslateAmt = 0;
      System.Drawing.Drawing2D.Matrix mTransformMat = new System.Drawing.Drawing2D.Matrix();
      

      bool mShowMouseTimes = false;
      Point mLastMousePos = new Point();

      int mDisplayXOffset = 10;
      string mDisplayStatsString = "";
      bool mDisplayClosePoint = false;
      Point mDisplayPointPos = new Point();
      float cMinCloseDist = 5.0f;


      static Brush mMainLineBrush = new SolidBrush(Color.FromArgb(53, 53, 53));
      static Brush mSupLineBrush = new SolidBrush(Color.FromArgb(45, 45, 45));
      static Pen mMainLinePen = new Pen(mMainLineBrush, 1);
      static Pen mSupLinePen = new Pen(mSupLineBrush, 1);

      //=========================================
      // fastTimeline
      //=========================================
      public fastTimeline()
      {
         InitializeComponent();
         this.SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
      }

      //=========================================
      // addNewGraphLine
      //=========================================
      public void addNewGraphLine(uint handle, string name, Color col)
      {
         for (int i = 0; i < mGraphLines.Count; i++)
         {
            if (mGraphLines[i].mHandle == handle)
            {
               return;
            }
         }

         GraphLine hl = new GraphLine();
         hl.mHandle = handle;
         hl.mName = name;
         hl.mColor = col;
         mGraphLines.Add(hl);
      }

      //=========================================
      // start
      //=========================================
      public void start()
      {
         mStartTime = DateTime.Now;
      }

      //=========================================
      // stop
      //=========================================
      public void stop()
      {
      }

       //=========================================
      // setLineEnabled
      //=========================================
      public void setLineEnabled(uint handle, bool onOff)
      {
         for (int i = 0; i < mGraphLines.Count; i++)
         {
            if (mGraphLines[i].mHandle == handle)
            {
               mGraphLines[i].mEnabled = onOff;
               return;
            }
         }
      }

      //=========================================
      // addPointToLine
      //=========================================
      public void addPointToLine(uint handle, int yValue)
      {
         for (int i = 0; i < mGraphLines.Count; i++)
         {
            if (mGraphLines[i].mHandle == handle)
            {
               mGraphLines[i].mYValues.Add(yValue);

               UpdateScales(mGraphLines[i].mYValues.Count, yValue);

               return;
            }
         }
      }

    
      //=========================================
      // UpdateScale
      //=========================================
      protected void UpdateScales(int xValue, int yValue)
      {
         if (yValue > mMaxYValue)
         {
            mMaxYValue = yValue + 2;
            mDrawDistanceBetweenYTicks = this.Height / mMaxYValue;
            if (mDrawDistanceBetweenYTicks == 0) mDrawDistanceBetweenYTicks = 1;
         }

         if (xValue > mMaxXValue)
         {
            mMaxXValue = xValue;
            //mDrawDistanceBetweenYTicks = this.Height / mMaxYValue;
         }
      }

      //=========================================
      // setScrollAmount
      //=========================================
      public void setScrollPercent(float normalizedPercent)
      {

         int maxScroll = Math.Max(this.Width, mMaxXValue * mDrawDistanceBetweenXTicks) - this.Width;


         if (normalizedPercent < 0) normalizedPercent = 0;
         if (normalizedPercent >= 1) normalizedPercent = 1;
         float scrollAmt = normalizedPercent * maxScroll;

         mTranslateAmt = -scrollAmt;// Math.Min(scrollAmt * mDrawDistanceBetweenXTicks, mMaxXValue * mDrawDistanceBetweenXTicks - this.Width);

         mTransformMat.Reset();
         mTransformMat.Translate(mTranslateAmt, 0);

      }



      //=========================================
      // OnPaint
      //=========================================
      protected override void OnPaint(PaintEventArgs e)
      {
         Graphics g = e.Graphics;

         g.Transform.Reset();
         drawBackground(g);

         g.Transform = mTransformMat;



         drawLines(g);
         drawText(g);

      }
      //=========================================
      // drawBackground
      //=========================================
      void drawBackground(Graphics g)
      {
         //fill our main background
         Rectangle rect = new Rectangle(0, 0, Width, Height);
         g.FillRectangle(GDIStatic.SolidBrush_CommonBGColor, rect);

         //draw our vertical lines
         if(mDrawDistanceBetweenXTicks>0)
         {
            int numX = Width / mDrawDistanceBetweenXTicks + 1;
            for (int x = 0; x < numX; x++)
               g.DrawLine(mMainLinePen, x * mDrawDistanceBetweenXTicks, 0, x * mDrawDistanceBetweenXTicks, Height);

            int megaStep = mDrawDistanceBetweenXTicks * 4;
            numX = Width / megaStep + 1;
            for (int x = 0; x < numX; x++)
               g.DrawLine(mSupLinePen, x * megaStep, 0, x * megaStep, Height);
         }



         //draw our horizontal lines
         if (mDrawDistanceBetweenYTicks > 0)
         {
            int numY = mDrawDistanceBetweenYTicks + 1;
            int stepY = Height / mDrawDistanceBetweenYTicks;

            for (int y = 0; y < numY; y++)
               g.DrawLine(mSupLinePen, 0, y * stepY, Width, y * stepY);
         }

      }

      //=========================================
      // drawLines
      //=========================================
      void drawLines(Graphics g)
      {
         int gridStep = mDrawDistanceBetweenXTicks;

         float maxX = 0;
         for (int i = 0; i < mGraphLines.Count; i++)
         {
            if (mGraphLines[i].mEnabled == false)
               continue;

            Point lastPt = new Point(0, Height);
            Pen linePen = new Pen(mGraphLines[i].mColor);

            for (int j = 0; j < mGraphLines[i].mYValues.Count; j++)
            {
               int yPt = (int)((mGraphLines[i].mYValues[j] / (float)mMaxYValue) * Height);

               Point currPt = new Point(j * gridStep, Height - yPt);
               g.DrawLine(linePen, lastPt, currPt);

               lastPt = currPt;
               if (currPt.X > maxX)
                  maxX = currPt.X;
            }
         }

         if (mDisplayClosePoint)
         {
            g.FillRectangle(GDIStatic.SolidBrush_White, mDisplayPointPos.X - 2, mDisplayPointPos.Y - 2, 4,4);
         }
      }

      //=========================================
      // drawText
      //=========================================
      void drawText(Graphics g)
      {
         //draw the 'time' at our current mouse position

         if (mShowMouseTimes)
         {
           // g.DrawString(mLastMousePos.X + "," + mLastMousePos.Y, GDIStatic.Font_Console_10, GDIStatic.SolidBrush_DimGray, mLastMousePos.X - mTranslateAmt + mDisplayXOffset, mLastMousePos.Y - 12);

            if (mDisplayClosePoint)
            {

               TimeSpan deltaTime = DateTime.Now - mStartTime;

               float minViewPort = -mTranslateAmt;
               float maxViewport = minViewPort + this.Width;
               float mouseX = minViewPort + mLastMousePos.X;
               float valPerc = mouseX / (mMaxXValue * mDrawDistanceBetweenXTicks);

    
               long subTicks = (long)(deltaTime.Ticks * valPerc);
               TimeSpan ts = new TimeSpan(subTicks);

               //convert milliseconds to hours, minutes, seconds
               int hours = (int)(ts.Hours);
               int minutes = (int)(ts.Minutes);
               int seconds = (int)(ts.Seconds);

               g.DrawString("  " + hours + ":" + minutes + ":" + seconds, GDIStatic.Font_Console_10, GDIStatic.SolidBrush_DimGray, mLastMousePos.X - mTranslateAmt + mDisplayXOffset, mLastMousePos.Y);

               

            
               g.DrawString(mDisplayStatsString, GDIStatic.Font_Console_10, GDIStatic.SolidBrush_DimGray, mLastMousePos.X - mTranslateAmt + mDisplayXOffset, mLastMousePos.Y + 12);

            }
         }

      }

      //=========================================
      // fastTimeline_MouseEnter
      //=========================================
      private void fastTimeline_MouseEnter(object sender, EventArgs e)
      {
         mShowMouseTimes = true;
      }

      //=========================================
      // fastTimeline_MouseLeave
      //=========================================
      private void fastTimeline_MouseLeave(object sender, EventArgs e)
      {
         mShowMouseTimes = false;
         mDisplayClosePoint = false;
         mDisplayStatsString = "";
      }

      //=========================================
      // fastTimeline_MouseMove
      //=========================================
      private void fastTimeline_MouseMove(object sender, MouseEventArgs e)
      {
         mLastMousePos.X = e.X;
         mLastMousePos.Y = e.Y;

         //reset these values
         mDisplayStatsString = "";
         mDisplayClosePoint = false;

         //find the closest line segment to us, and update our display information.
         {
            float minDist = float.MaxValue;

            float minViewPort = -mTranslateAmt;
            float maxViewport = minViewPort + this.Width;
            float mouseX = minViewPort + mLastMousePos.X;


            int stepIndex = (int)(mouseX / mDrawDistanceBetweenXTicks);
            int mouseY = (int)((Height - mLastMousePos.Y));// / mDrawDistanceBetweenYTicks);

            bool foundClose = false;
            for (int i = 0; i < mGraphLines.Count; i++)
            {
               if (mGraphLines[i].mEnabled == false)
                  continue;

               if (stepIndex >= mGraphLines[i].mYValues.Count)
                  continue;

               int yVal0 = mGraphLines[i].mYValues[stepIndex];
               int yPt = (int)( (yVal0 / (float)mMaxYValue) * Height);


               float d = Math.Abs(mouseY - (yPt));
               if (d > cMinCloseDist)
                  continue;

               foundClose = true;
               if (d < minDist)
               {
                  minDist = d;
                  mDisplayPointPos.X = (int)(stepIndex * mDrawDistanceBetweenXTicks);

                  

                  mDisplayPointPos.Y = Height - yPt;

                  mDisplayStatsString = "  " + yVal0 + "mb";
                  mDisplayClosePoint = true;

               }


            }
         }

         this.Refresh();
      }
   }
}
