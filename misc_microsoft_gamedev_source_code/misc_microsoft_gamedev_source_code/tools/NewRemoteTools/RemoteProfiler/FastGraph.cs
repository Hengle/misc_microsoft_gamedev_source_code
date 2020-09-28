using System;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;

using RemoteTools;

namespace EnsembleStudios.RemoteGameDebugger.Profiler
{
	public class FastGraph : EnsembleStudios.RemoteGameDebugger.DXManagedControl
	{
		private System.ComponentModel.IContainer components = null;

		public FastGraph()
		{
			InitializeComponent();

         this.mBackgroundColor = System.Drawing.Color.DarkSlateGray.ToArgb();
         mMouseTrackerVisual = new MouseTrackerVisual(this);
         
		}

      MouseTrackerVisual mMouseTrackerVisual;
      public float mRange = 1000;

      //float mOneMS = 0.001f;
      static float mOneMS = 10000;
      float mDomain = 120 * mOneMS;

      //public float mDomain = 0.07f;
      public float mXOffset = 0;
      public float mYOffset = 0;
      
      protected int mMouseX = 0;
      protected int mMouseY = 0;
      protected bool mMouseDown = false;

      //public ArrayList mScalarPlots = new ArrayList();
      public ScalarPlotData[] mScalarPlots = null;
      public ArrayList mFrames = null;
   
      public bool mbShowLatestData = true;

      const float cNewestFrameLimit = 0.9f;

      TimeLineMarker mTopFrameMarker = null;
      TimeLineMarker mBottomFrameMarker = null;


      public void UpdateGraphData()
      {
         this.UpdateIfNeeded();
         //Render();
      }
      public void SetScalarPlots(ScalarPlotData[] plots, ArrayList frames)
      {
         mScalarPlots = plots;
         mFrames = frames;
      }
      public void ClearScalarPlots()
      {
         mScalarPlots = null;
      }
      public void Clear()
      {
         ClearScalarPlots();
         mFrames = null;
         UpdateIfNeeded();
      }
      public void SetTopViewLive()
      {
         if(mFrames != null)
         {
            mTopFrameMarker = new TimeLineMarker(mFrames.Count-1);
         }
      }

      #region Scroll and Zoom code

      //protected ScrollBar mFrameViewZoomScrollBar;
      protected ScrollBar mHorizontalScrollBar;

      public void SetScrollBars( ScrollBar hScroll)
      {
         mHorizontalScrollBar = hScroll;
      }

      //float cScrollFactor = 10;

      public void ScrollX(int scrollInt, int outOf)
      {
         float mXScrollOffset = 0;
         
         //mXScrollOffset = cScrollFactor * ((scrollInt - (0.5f * outOf))/(float)outOf);
         mXScrollOffset = scrollInt;
         mXOffset = mXScrollOffset;
         this.UpdateIfNeeded();

         if(mFrames != null)
         {
            mHorizontalScrollBar.Minimum = 0;
            mHorizontalScrollBar.SmallChange = 1;
            mHorizontalScrollBar.LargeChange = mFrames.Count / 10;
            mHorizontalScrollBar.Maximum = mFrames.Count;
         }
      }

      #endregion

      #region Draw Code

      public bool mbTimeMarkers = true;
      

      public override void Draw()
      {
         try
         {
            if(mFrames == null)
               return;
            if(mbShowLatestData && (mFrames.Count > (mRange * cNewestFrameLimit + mXOffset)))
            {
               mXOffset = mFrames.Count - 900;
               //mHorizontalScrollBar.Value = mFrames.Count;           
            }
            else
            {

            }


            RectangleF graphRect = new RectangleF(0,0,mRange*mZoomX,mDomain*mZoomY);
            SetView(graphRect );         
         
            int gridColor = System.Drawing.Color.Gray.ToArgb();
            RectangleF grid = new RectangleF(0,0,mRange*mZoomX,mDomain*mZoomY);
            DrawGrid(gridColor,grid,-mXOffset,100,mYOffset,mOneMS * 10);
            FlushAll();

            int yScale = 5;//ms
            RectangleF scale = new RectangleF(0,0,25*mZoomX,mDomain*mZoomY);
            RectangleF scale2 = new RectangleF(975*mZoomX,0,25,mDomain*mZoomY);
            int scaleColor = System.Drawing.Color.White.ToArgb();
            DrawScale(scaleColor,scale,0,mOneMS * yScale,false,true,true);      
            DrawScale(scaleColor,scale2,0,mOneMS * yScale,false,true,false);

            if (mbTimeMarkers)
            {
               //Draw 30 and 60 ms lines
               DrawLine(Color.LightGreen.ToArgb(), 0, mOneMS * 30, mRange * mZoomX, mOneMS * 30);
               DrawLine(Color.Orange.ToArgb(), 0, mOneMS * 60, mRange * mZoomX, mOneMS * 60);
               int px;
               int py;
               GetPixel(0, mOneMS * 30, out px, out py);
               if (py > 0) 
                  DrawText(this.Right - 50, py, "30 ms", System.Drawing.Color.LightGreen.ToArgb());
               GetPixel(0, mOneMS * 60, out px, out py);
               if (py > 0) 
                  DrawText(this.Right - 50, py, "60 ms", System.Drawing.Color.Orange.ToArgb());
            }
            else
            {
               //Draw 50 and 100%
               DrawLine(Color.LightGreen.ToArgb(), 0, mOneMS * 100, mRange * mZoomX, mOneMS * 100);
               DrawLine(Color.Orange.ToArgb(), 0, mOneMS * 50, mRange * mZoomX, mOneMS * 50);
               int px;
               int py;
               GetPixel(0, mOneMS * 100, out px, out py);
               if (py > 0) 
                  DrawText(this.Right - 50, py, "100 %", System.Drawing.Color.LightGreen.ToArgb());
               GetPixel(0, mOneMS * 50, out px, out py);
               if (py > 0) 
                  DrawText(this.Right - 50, py, "50 %", System.Drawing.Color.Orange.ToArgb());

            }

            FlushAll();


            mMouseTrackerVisual.Draw(mMouseDown,mMouseX,mMouseY);
        
            //DrawText(this.Width/2,this.Height/2,"!!!!");

            RectangleF worldView = new RectangleF(mXOffset, mYOffset, mRange * mZoomX, mDomain * mZoomY);         
            SetView(worldView );
         
            if(mScalarPlots == null)
               return;

            foreach(ScalarPlotData s in mScalarPlots)
            {
               float[] data = (float[])s.getData().ToArray(typeof(float));
               PlotScalarData(worldView,s.mColor,data,(int)mXOffset,(int)(mXOffset+mRange));
            }
            FlushAll();

            int i = 40;
            string name;
            foreach(ScalarPlotData s in mScalarPlots)
            {                  
               name = s.mName.Replace("_"," ");
               DrawText(i,0,name, s.mColor);
               i+=100;               
            }

            DrawText(i+100,0,String.Format("Scale: {0} frames wide by {1} ms tall",mRange*mZoomX,mDomain*mZoomY/mOneMS), System.Drawing.Color.White.ToArgb());

            SetView(worldView );
            if(mTopFrameMarker != null)
            {
               float xpos = mTopFrameMarker.GetMarkerTime();
               DrawLine(System.Drawing.Color.Red.ToArgb(),xpos,0,xpos+1,Constants.cUIntScaleToMS);
            }
            if(mBottomFrameMarker != null)
            {
               float xpos = mBottomFrameMarker.GetMarkerTime();
               DrawLine(System.Drawing.Color.Cyan.ToArgb(),xpos,0,xpos+1,Constants.cUIntScaleToMS);
            }         
         }
         catch(System.Exception ex)
         {
            ErrorHandler.Error(ex.ToString());
            //todo: log this error
            //MessageBox.Show(this,"Error Drawing Frame Timeline: " + ex.ToString());
         }

      }

      //add starting index support to avoid extra work
      //when scaling is added to the x axis ignore some data when zoomed out?

      protected void PlotData(RectangleF rect, int color, PointF[] data)
      {
         PlotData(rect,color,data,0,data.Length-1);
      }
      protected void PlotData(RectangleF rect, int color, PointF[] data, int StartIndex, int EndIndex)
      {
         if(EndIndex > data.Length)
         {
            EndIndex = data.Length;
         }
         for(int i = StartIndex; i < EndIndex-1; i++)
         {            
            DrawLine(color,data[i].X,data[i].Y,data[i+1].X,data[i+1].Y);
         }
      }
      protected void PlotScalarData(RectangleF rect, int color, float[] data)
      {
         PlotScalarData(rect,color,data,0,data.Length-1);
      }

      protected void PlotScalarData(RectangleF rect, int color, float[] data, int StartIndex, int EndIndex)
      {
         if(EndIndex > data.Length)
         {
            EndIndex = data.Length;
         }
         float xpos;
         for(int i = StartIndex; i < EndIndex-1; i++)
         {    
            xpos = (float)i;
            DrawLine(color,xpos,data[i],xpos+1,data[i+1]);
         }
      }

      #endregion

		#region Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
         // 
         // FastGraph
         // 
         this.Name = "FastGraph";
         this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.FastGraph_MouseUp);
         this.Paint += new System.Windows.Forms.PaintEventHandler(this.FastGraph_Paint);
         this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.FastGraph_KeyUp);
         this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.FastGraph_KeyDown);
         this.MouseMove += new System.Windows.Forms.MouseEventHandler(this.FastGraph_MouseMove);
         this.MouseDown += new System.Windows.Forms.MouseEventHandler(this.FastGraph_MouseDown);

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
      
      #endregion

      #region UI Code
      private void FastGraph_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
      {
         mMouseX = e.X;
         mMouseY = e.Y;
         UpdateIfNeeded();
         
         if(mMouseDown == true)
            OnFrameSelected(e.Button);
      }
      private void FastGraph_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
      {
         mMouseDown = true;
         OnFrameSelected(e.Button);
         UpdateIfNeeded();

      }
      private void FastGraph_MouseUp(object sender, System.Windows.Forms.MouseEventArgs e)
      {
         mMouseDown = false;
         UpdateIfNeeded();
      }

      public delegate void FrameHandler(object sender, Frame f, MouseButtons context);
      
      public event FrameHandler OldFrameSelected = null;
      public event FrameHandler NewestFrameSelected = null;

      protected void OnFrameSelected(MouseButtons context)
      {
         if(mFrames == null)
            return;
         int nearestFrame =  (int)((mMouseX * mRange / this.Width) + mXOffset);
         if((mFrames.Count > nearestFrame) && (nearestFrame >= 0))
         {
            Frame f = (Frame)(mFrames[nearestFrame]);

            if (OldFrameSelected != null)
               OldFrameSelected(this, f, context);  

            if(context == MouseButtons.Left)
            {
               mTopFrameMarker = new TimeLineMarker(nearestFrame);
            }
            else if(context == MouseButtons.Right)
            {
               mBottomFrameMarker = new TimeLineMarker(nearestFrame);
            }
         }
         else
         {
            if((NewestFrameSelected != null) && (mFrames.Count > 1))
               NewestFrameSelected(this, (Frame)(mFrames[mFrames.Count-1]),context);

            if(context == MouseButtons.Right)
            {
               mBottomFrameMarker = new TimeLineMarker(mFrames.Count - 1);
            }
         }    
      }

      bool mbShiftOn = false;
      private void FastGraph_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
      {
         mbShiftOn = e.Shift;

         if (e.KeyCode == Keys.Q)
         {
            mMouseX--;
            OnFrameSelected(MouseButtons.Left);
            UpdateIfNeeded();

         }
         else if (e.KeyCode == Keys.W)
         {
            mMouseX++;
            OnFrameSelected(MouseButtons.Left);
            UpdateIfNeeded();
         }
      }

      private void FastGraph_KeyUp(object sender, System.Windows.Forms.KeyEventArgs e)
      {
         mbShiftOn = e.Shift;
      
      }

      private void FastGraph_Paint(object sender, System.Windows.Forms.PaintEventArgs e)
      {
         this.UpdateIfNeeded();
      }
      #endregion

	}

   //x coordinate is the sample# and the y coordinate is a scalar value.
   public class ScalarPlotData
   {
      public string mName;
      public int mColor;
      public ArrayList mData = new ArrayList();
      public DrawStyle mStyle = DrawStyle.Normal;

      public enum DrawStyle
      {
         Normal,
         Dashed
      }
      public ScalarPlotData(string name, int color)
      {
         mData = new ArrayList();
         mName = name;
         mColor = color;
      }
      public ArrayList getData()
      {
         return mData;
      }
      public void Add(float data)
      {
         mData.Add(data);
      }
      public void Add(double data)
      {
         mData.Add((float)data);
      }
   }


}

