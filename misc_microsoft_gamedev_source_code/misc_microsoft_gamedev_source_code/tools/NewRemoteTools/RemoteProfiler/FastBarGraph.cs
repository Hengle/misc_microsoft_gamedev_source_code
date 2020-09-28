using System;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;

using RemoteTools;

namespace EnsembleStudios.RemoteGameDebugger.Profiler
{
	public class FastBarGraph : EnsembleStudios.RemoteGameDebugger.DXManagedControl
	{
      
		private System.ComponentModel.IContainer components = null;

		public FastBarGraph()
		{
			InitializeComponent();
         this.mBackgroundColor = System.Drawing.Color.DarkSlateGray.ToArgb();

		}
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
         // 
         // FastBarGraph
         // 
         this.BackColor = System.Drawing.SystemColors.AppWorkspace;
         this.Name = "FastBarGraph";
         this.Size = new System.Drawing.Size(184, 168);
         this.Click += new System.EventHandler(this.FastBarGraph_Click);
         this.Paint += new System.Windows.Forms.PaintEventHandler(this.FastBarGraph_Paint);
         this.MouseMove += new System.Windows.Forms.MouseEventHandler(this.FastBarGraph_MouseMove);

      }
		#endregion


      //MouseTrackerVisual mMouseTrackerVisual;
      public float mRange = 150 * mOneMS;

      //float mOneMS = 0.001f;
      static float mOneMS = 10000;
      float mDomain = 1100;

      //public float mDomain = 0.07f;
      public float mXOffset = 0;
      public float mYOffset = 0;
      
      protected int mMouseX = 0;
      protected int mMouseY = 0;
      protected bool mMouseDown = false;

      Bar[] mData = new Bar[0];

      public enum StatMode
      {
         cAverageTotal,
         cAverageNSamples,
         cNoAverage
      }
      public StatMode mMode = StatMode.cNoAverage;

      ArrayList mDrawOrder = new ArrayList();

      public bool mbShowAverage = false;
      public bool mbSortList = false;
      public bool mbDiffView = false;
      public bool mbZoom = false;

      int mDataCount;
      int mDisplayCount;
      public void SetData(Bar[] data, int count)
      {
         mData = data;
         mDataCount = count;
      }
      public void Clear()
      {
         mData = null;
         mDataCount = 0;
         UpdateIfNeeded();
      }


      #region Draw Code
      public override void Draw()
      {
         try
         {
            mDisplayCount = 20;
            //mDisplayCount = mDataCount;

            //zoom in vs show all settings
            float offset = 0;
            float offsetAmount = 50f;// this is the bar size
            float yOffset = mYScrollPercent * mDomain;
            if (mbZoom == false)
            {
               mZoomY = 1000f / (mDataCount * 50);
               mZoomY = 1 / mZoomY;
               //offsetAmount = 1000f / mDataCount;
               yOffset = 0;
            }
            else
            {
               mZoomY = 1;
            }

            //RectangleF graphRect = new RectangleF(0, 0, mRange * mZoomX, mDomain * mZoomY);
            RectangleF graphRect = new RectangleF(0, yOffset, mRange * mZoomX, mDomain * mZoomY);

            if(mbDiffView == true)
            {
               graphRect = new RectangleF(mRange*mZoomX * -0.5f,0,mRange*mZoomX,mDomain*mZoomY);
            }

         
            //int gridColor = System.Drawing.Color.Gray.ToArgb();
            RectangleF grid = new RectangleF(0,0,mRange*mZoomX,this.Height);
            //DrawGrid(gridColor,grid,-mXOffset,100,mYOffset,mOneMS * 10);
            
            RectangleF scale = new RectangleF(0,this.Height - 50,mRange*mZoomX,10);
            if(mbDiffView == true)
            {
               scale.Offset(-0.5f * mRange * mZoomX,0);
               grid.Offset(-0.5f * mRange * mZoomX,0);
            }
            SetView(grid);         
            DrawScale(System.Drawing.Color.White.ToArgb(),scale,0,5*mOneMS,true,true,false);
            FlushAll();

            SetView(graphRect );         

            if(mDataCount == 0)
               return;

            mDrawOrder.Clear();
            //calculate values
            for(int j=0; j<mDataCount; j++)
            {
               Bar b = mData[j];
               if(mMode == StatMode.cAverageTotal)
               {
                  b.mLastValue = b.GetAverage();
               }
               else if(mMode == StatMode.cAverageNSamples)
               {
                  b.mLastValue = b.GetMovingAverage();
               }               
               else
               {
                  b.mLastValue = b.mValue;
               }
               if(mbSortList == true)
               {
                  if(b.mLastValue == 0)
                  {
                     mDrawOrder.Add(b);
                  }
                  else
                  {
                     int index = 0;
                     int insertIndex = 0;
                     foreach(Bar b2 in mDrawOrder)
                     {
                        if(b.mLastValue < b2.mLastValue)
                        {
                           insertIndex = index + 1;
                        }
                        index++;
                     }
                     mDrawOrder.Insert(insertIndex,b);
                  }
               }
               else
               {
                  mDrawOrder.Add(b);
               }
            }

            //foreach(Bar b in mData)
            //for(int i=0; i<mDataCount; i++)

            int peakColor = System.Drawing.Color.White.ToArgb();
            int white = System.Drawing.Color.White.ToArgb();
            int black = System.Drawing.Color.Black.ToArgb();
            float peakWidth =   3f * mRange*mZoomX / this.Width;
            int i = 0;
            foreach(Bar b in mDrawOrder)
            {
               //Bar b = mData[i];
               //offset = 50 + offsetAmount * sample.mLevel;
               offset = i * offsetAmount;
               float top = offset;
               float bottom = offset + offsetAmount;

//               if(mSelected == i)
//               {
//                  top = top - 3;
//                  bottom = bottom + 3;
//               }
               peakColor = white;

               if(mSelectedBars.Contains(b.mName))
               {
                  DrawBox(Color.DarkSlateBlue.ToArgb(),top , -mRange*mZoomX, mRange*mZoomX, bottom );
                  peakColor = Color.Yellow.ToArgb();
               }
 
               DrawBox(b.mColor,top , 0, (float)b.mLastValue, bottom );
               DrawBox(peakColor,top , (float)b.mPeakValue, (float)b.mPeakValue + peakWidth, bottom );
              
               i++;
            }
            FlushAll();

            i = 0;
            int textcolor1;
            int textcolor2;
            foreach (Bar b in mDrawOrder)
            {
               offset = i * offsetAmount;
               int px;
               int py;

               if (mbZoom == true)
               {
                  if (GetPixel(0, offset + 40, out px, out py))
                  {
                     Color c;
                     c = Color.FromArgb(b.mColor);
                     if (c.R * c.R * 0.7 + c.G * c.G * 1.4 + c.B * c.B * 0.5 > 255 * 255)
                     {
                        textcolor1 = textcolor2 = black;
                     }
                     else
                     {
                        textcolor1 = textcolor2 = white;
                     }
                     if (this.GetWorldX(40) > b.mValue)
                        textcolor1 = white;

                     if (this.GetWorldX(100) > b.mValue)
                        textcolor2 = white;

                     double val = b.mValue / 10000;
                     DrawText(40, py, val.ToString("F01"), textcolor1);
                     DrawText(100, py, b.mName, textcolor2);
                  }
               }

               i++;
            }

            FlushAll();

            this.SetViewToPixels();
            DrawBox(System.Drawing.Color.Black.ToArgb(),this.Height - 40 , 0, this.Width, this.Height  );
            FlushAll();
        
            if((mSelected >= 0) && (mSelected < mDataCount))
            {
               //Bar b = mData[mSelected];
               Bar b = (Bar)(mDrawOrder[mSelected]);
               DrawText(
                  0, 
                  0,
                  b.mName.ToString(),
                  b.mColor);
               double displayValue = 0;
               if(mbShowAverage == true)
               {
                  if(b.mCount != 0)
                  {
                     displayValue = (b.mTotal / b.mCount) / mOneMS;
                  }
                  else
                  {
                     displayValue = 0;
                  }
               }
               else
               {
                  displayValue = b.mValue / mOneMS;
               }

               DrawText(
                  0, 
                  13,
                  String.Format("{0:0.0000}",displayValue),
                  System.Drawing.Color.White.ToArgb());


            }
            DrawText(
               0, 
               26,
               String.Format("{0} - {1}ms",scale.Left / mOneMS,scale.Right / mOneMS ),
               System.Drawing.Color.White.ToArgb());
         }
         catch(System.Exception ex)
         {
            ErrorHandler.Error(ex.ToString());
         }

      }
      #endregion

      float cScrollFactor = 10;
      float mYScrollPercent = 0;

      public void ScrollY(int scrollInt, int outOf)
      {
         float mXScrollOffset = 0;
         //if (mCurrentFrame == null)
         //   return;
         //mXScrollOffset = cScrollFactor * ((scrollInt - (0.5f * outOf)) / (float)outOf);
         //mYOffset = (float)mCurrentFrame.getCPUStartTime() + mXScrollOffset * this.mRange;

         mYScrollPercent = 1 - (scrollInt / (float)outOf);

         this.UpdateIfNeeded();
      }


      private void FastBarGraph_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
      {
         mMouseX = e.X;
         mMouseY = e.Y;
         UpdateIfNeeded();
         SelectSection();
      }
      private float GetWorldY(int pixelY)
      {
         return (float)((pixelY * mDomain / this.Height) + mYOffset);
      }
      private float GetWorldX(int pixelX)
      {
         return (float)((pixelX * mRange / this.Width) + mXOffset);
      }


      int mSelected = -1;
      private void SelectSection()
      {
         float y = GetWorldY(this.Height - mMouseY);    
         //float offsetAmount = 1000f/mDataCount;
         //offsetAmount = 1000 / mDataCount * 50f;
         //mSelected = (int)(y / offsetAmount);

         float yOffset = mYScrollPercent * mDomain;
         if (mbZoom == false)
         {
            yOffset = 0;
         }
         float a = mZoomY * ((yOffset + y) / 50);
         mSelected = (int)a;
      }

      private void FastBarGraph_Paint(object sender, System.Windows.Forms.PaintEventArgs e)
      {
         this.UpdateIfNeeded();
      }

      private void FastBarGraph_Click(object sender, System.EventArgs e)
      {
         if(mDrawOrder == null || mDrawOrder.Count == 0)
            return;
         if (mSelected >= mDrawOrder.Count)
            return;

         Bar b = (Bar)(mDrawOrder[mSelected]);
         ToggleByName(b.mName);
      }
      public void ToggleByName(string barName)
      {
         lock(mSelectedBars.SyncRoot)
         {
            if(!mSelectedBars.Contains(barName))
            {
               mSelectedBars.Add(barName);
            }
            else
            {
               mSelectedBars.Remove(barName);                                           
            }
         }
      }
      public ArrayList mSelectedBars = new ArrayList();
	}

   public class Bar
   {
      public string mName;
      public double mValue;
      public int mColor = 0;

      public Bar()
      {
         mAverageValues = new StatBuffer(5);
      }
      public Bar(int historySize)
      {
         mAverageValues = new StatBuffer(historySize);
      }
      public bool SetValue(double val, bool newValue)
      {
         mValue = val;
         if(newValue)
         {
            mCount++;
            mTotal += val;
            mAverageValues.InsertValue(val);
            if(mValue > mPeakValue)
            {
               mPeakValue = val;
               return true;
            }
         }
         return false;
      }
      public double GetMovingAverage()
      {
         return mAverageValues.GetAverage();
      }
      public double GetAverage()
      {
         if(mCount != 0)
            return (mTotal / mCount);
         else
            return 0;
      }

      public double mLastValue = 0;

      public double mPeakValue = 0;
      public double mTotal = 0; 
      public int mCount = 0;  

      StatBuffer mAverageValues;

   }
}

