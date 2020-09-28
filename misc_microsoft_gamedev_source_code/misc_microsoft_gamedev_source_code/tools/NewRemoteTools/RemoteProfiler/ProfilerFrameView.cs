using System;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;

using RemoteTools;

namespace EnsembleStudios.RemoteGameDebugger
{
	public class ProfilerFrameView : EnsembleStudios.RemoteGameDebugger.DXManagedControl
	{
		private System.ComponentModel.IContainer components = null;

		public ProfilerFrameView() 
		{
    
			InitializeComponent();

         this.mBackgroundColor = System.Drawing.Color.DarkSlateGray.ToArgb() - 10;
         
         //Setup mouse regions (in world coordinates)
         mMouseTrackerVisual = new MouseTrackerVisual(this);

         //Setup light buttons (in screen 
//         mCloseGPUStack = new UIRegion("GPUStackCloseButton", new RectangleF(0,0,0,0));
//         mCloseCPUStack = new UIRegion("CPUStackCloseButton", new RectangleF(0,0,0,0));
//         mCloseGPUStack.Clicked+=new MouseRegionEvent(mCloseGPUStack_Clicked); 
//         mCloseCPUStack.Clicked+=new MouseRegionEvent(mCloseCPUStack_Clicked);
//         this.RegisterButton(mCloseGPUStack);
//         this.RegisterButton(mCloseCPUStack);

         //Set zoom properties
         mZoomInExponent = 14;//12//was 10;//8;
         mZoomOutExponent = 16;//was 8;//4;

         this.MouseWheel+=new MouseEventHandler(ProfilerFrameView_MouseWheel);
      }

      UIRegion mCloseGPUStack;
      UIRegion mCloseCPUStack;

      TimeLineMarker mMarker = null;

      static float mOneMS = 10000;
      float mRange = 33 * mOneMS;
      float mDomain = 1050; //units
      float mXOffset = 0;
      float mYOffset = 0; //always 0      
      
      protected int mMouseX = 0;
      protected int mMouseY = 0;
      protected bool mMouseDown = false;

      BRetiredProfileSample mSampleUnderMouse = null;


      MouseTrackerVisual mMouseTrackerVisual;
      Frame mCurrentFrame = null;

      ArrayList mSelectedCallStackCPU = new ArrayList();
      ArrayList mSelectedCallStackGPU = new ArrayList();
      BProfileSection[] mSections;
      int mNumSections;

      ResourceHandle mCurrentFrameHandle = null;

      Frame mLastFrame = null;
      ResourceHandle mLastFrameHandle = null;
      ArrayList mFrames = new ArrayList();
      ArrayList mFrameHandles = new ArrayList();

      public void SetFrame(Frame frame, ref BProfileSection[] sections, int numSections)
      {
         lock(this)
         {
            mLastFrame = mCurrentFrame;
            mLastFrameHandle = mCurrentFrameHandle;

            mCurrentFrameHandle = frame.SafeGetBufferHandle();

            mCurrentFrame = frame;
            mSections = sections;
            mNumSections = numSections;

//            if(mGPUMarker != null)
//            {
//               mSelectedCallStackGPU = FindSamples(mGPUMarker.GetMarkerTime(),SampleType.GPU);
//            }
//            if(mCPUMarker != null)
//            {
//               mSelectedCallStackCPU = FindSamples(mCPUMarker.GetMarkerTime(),SampleType.CPU);
//            }
         }

         this.UpdateIfNeeded();   
      }

      public void Clear()
      {
         mLastFrame = null;
         mLastFrameHandle = null;

         mCurrentFrameHandle = null;

         mCurrentFrame = null;
         mSections = null;
         mNumSections = 0;
         this.UpdateIfNeeded();   

      }

      #region Scroll and Zoom code

      protected ScrollBar mFrameViewZoomScrollBar;
      protected TrackBar mFrameViewZoomYBar;
      protected ScrollBar mFrameViewHorizontalScrollBar;

      public void SetScrollBars(ScrollBar zoomBar, ScrollBar hScroll)
      {
         mFrameViewHorizontalScrollBar = hScroll;
      }
      public void SetZoomBars(TrackBar zoomX, TrackBar zoomY)
      {
         mFrameViewZoomYBar = zoomY;
      }

      float cScrollFactor = 10;

      public void ScrollX(int scrollInt, int outOf)
      {
         float mXScrollOffset = 0;
         if(mCurrentFrame == null)
            return;
         mXScrollOffset = cScrollFactor * ((scrollInt - (0.5f * outOf))/(float)outOf);
         mXOffset = (float)mCurrentFrame.getCPUStartTime() + mXScrollOffset * this.mRange;
         this.UpdateIfNeeded();
      }

      //this fancy function zooms to the location of the cursor!
      public void WheelZoom(MouseEventArgs e)
      {
         if (mFrameViewZoomYBar == null)
            return;
         float oldZoom = GetZoomX();

         mFrameViewZoomYBar.Value = (int)Math.Max(Math.Min(mFrameViewZoomYBar.Value + (e.Delta / 10), mFrameViewZoomYBar.Maximum), mFrameViewZoomYBar.Minimum);

//         int valueDelta = (int)(e.Delta / 10);
//         if(mFrameViewZoomScrollBar.Value + valueDelta < mFrameViewZoomScrollBar.Minimum)
//            mFrameViewZoomScrollBar.Value = mFrameViewZoomScrollBar.Minimum;
//         else if(mFrameViewZoomScrollBar.Value + valueDelta > mFrameViewZoomScrollBar.Maximum)
//            mFrameViewZoomScrollBar.Value = mFrameViewZoomScrollBar.Maximum;
//         else
//            mFrameViewZoomScrollBar.Value += valueDelta;

         float oldmousex = GetMouseWorldLocation();
         ZoomX(mFrameViewZoomYBar.Value, false);
         float newmousex = GetMouseWorldLocation();
         SetMouseWorldLocation(oldmousex, e.X);
         UpdateIfNeeded();
         

      }

      public void SetMouseWorldLocation(float mWorldLoc, int mouseX)
      {
         int left = mouseX - this.Left;
         float p = left / (float)this.Width;
         this.mXOffset = -mRange*mZoomX * p + mWorldLoc;

         //Update the horizontal scroll bar.
         if(mFrameViewHorizontalScrollBar == null || mCurrentFrame == null)
            return;
         float hscrollValue = (mXOffset - (float)mCurrentFrame.getCPUStartTime()) / this.mRange;
         int outof = mFrameViewHorizontalScrollBar.Maximum;
         int newPos = (int)(hscrollValue *  1/cScrollFactor * outof + 0.5f * outof);
         if(newPos > mFrameViewHorizontalScrollBar.Maximum)
         {
            newPos = mFrameViewHorizontalScrollBar.Maximum;
         }
         else if(newPos < mFrameViewHorizontalScrollBar.Minimum)
         {
            newPos = mFrameViewHorizontalScrollBar.Minimum;
         }
         mFrameViewHorizontalScrollBar.Value = newPos;
      }

      #endregion

      #region Draw Code     

      public class FrameDataRegion : UIRegion
      {
         public FrameDataRegion(string name, RectangleF rect, Frame frame) : base(name, rect)
         {
            mFrame = frame;
         }
         public FrameDataRegion(string name, RectangleF rect, int threadID) : base(name, rect)
         {
            mThreadId = threadID;
         }
         public int mThreadId = -1;
         public Frame mFrame = null;
      }

      UIRegion mTopScaleRegion;
      UIRegion mBottomScaleRegion;
      UIRegion mGraphRegion;

      UIRegion mTextRegion;
      FrameDataRegion mGPURegion;
      FrameDataRegion mGPUConnectionRegion;
      FrameDataRegion mMainThreadRegion;
      ArrayList mOtherThreadRegions = new ArrayList();
      //ArrayList mOtherCPUs = new ArrayList();
      ArrayList mAllSampleDataRegions = new ArrayList();

      ArrayList mCPURegion = new ArrayList();
      ArrayList mThreadIDs = new ArrayList();

      //display options
      ViewType mViewType = ViewType.PerCPUView;
      DataRenderOptions mRenderOptions = DataRenderOptions.Full;

      public string ViewTypeLayout
      {
         set
         {
            mViewType = (ViewType)Enum.Parse(typeof(ViewType),value,true);
            UpdateIfNeeded();
         }
         get
         {
            string s = mViewType.ToString();
            return s;
         }
      }
      public string DrawTypeLayout
      {
         set
         {
            mRenderOptions = (DataRenderOptions)Enum.Parse(typeof(DataRenderOptions),value,true);
            UpdateIfNeeded();
         }
         get
         {
            string s = mRenderOptions.ToString();
            return s;
         }
      }

      public enum ViewType
      {
         PerCoreView,
         PerCPUView,
         ThreadView
      }
      public enum DataRenderOptions
      {
         Solid,
         Full
      }

      private bool mbDefaultSet = false;
      private void SetupDefaultRegions()
      {
         float left = 0.0f;
         // remember that 0,0 is lower left... ugh
//         mTextRegion = new UIRegion("TextRegion",RectangleF.FromLTRB(left,1000/mDomain,1,1060/mDomain));
//         mBottomScaleRegion = new UIRegion("TopScale",RectangleF.FromLTRB(left,0,1,50/mDomain));
//         mTopScaleRegion = new UIRegion("BottomScale",RectangleF.FromLTRB(left,950/mDomain,1,1000/mDomain));
//         mGraphRegion  = new UIRegion("GraphRegion",RectangleF.FromLTRB(left,50/mDomain,1,950/mDomain));

         if(mbDefaultSet == false)
         {
            mbDefaultSet = true;
         
            mTextRegion = new UIRegion("TextRegion",RectangleF.FromLTRB(left,0,1,50/mDomain));
            mBottomScaleRegion = new UIRegion("TopScale",RectangleF.FromLTRB(left,50/mDomain,1,100/mDomain));
            mTopScaleRegion = new UIRegion("BottomScale",RectangleF.FromLTRB(left,1000/mDomain,1,1050/mDomain));
            mGraphRegion  = new UIRegion("GraphRegion",RectangleF.FromLTRB(left,100/mDomain,1,1000/mDomain));
         }
      }

      private void SetupRegions()
      {
         mAllSampleDataRegions.Clear();
         SetupDefaultRegions();

         float left = 0.0f;
         // remember that 0,0 is lower left... ugh
         float bottom = mGraphRegion.mRect.Bottom;
         float top = bottom - (mGraphRegion.mRect.Height * 0.20f);
         mGPURegion = new FrameDataRegion("GPU",RectangleF.FromLTRB(left,top,1,bottom),mCurrentFrame);
         mAllSampleDataRegions.Add(mGPURegion);
         
         bottom = mGPURegion.mRect.Top;
         top = bottom - (mGraphRegion.mRect.Height * 0.05f);
         mGPUConnectionRegion = new FrameDataRegion("GPUConnection",RectangleF.FromLTRB(left,top,1,bottom),mCurrentFrame);      
         
         if(mViewType == ViewType.ThreadView)
         {  
            //sort over multiple frames.
            mOtherThreadRegions.Clear();  

            //mCurrentFrame.mThreadFrames.Sort(new Frame.SortByThreadID());

            foreach(Frame Sub in mCurrentFrame.mThreadFrames)
            {            
               if(!mThreadIDs.Contains(Sub.mThreadID))
               {
                  mThreadIDs.Add(Sub.mThreadID);
               }
            } 
            foreach(Frame otherFrame in mOtherFrames)
            {
               foreach(Frame Sub in otherFrame.mThreadFrames)
               {            
                  if(!mThreadIDs.Contains(Sub.mThreadID))
                  {
                     mThreadIDs.Add(Sub.mThreadID);
                  }
               }
            }

            bottom = mGPUConnectionRegion.mRect.Top;
            top = bottom - (mGraphRegion.mRect.Height * 0.25f);
            mMainThreadRegion = new FrameDataRegion("MainThread",RectangleF.FromLTRB(left,top,1,bottom),mCurrentFrame);            
            mAllSampleDataRegions.Add(mMainThreadRegion);
         
            if(mCurrentFrame != null && mCurrentFrame.mThreadFrames.Count > 0)
            {           
               float size = 0.10f;
               UIRegion lastRegion = mMainThreadRegion;
               if(mThreadIDs.Count > 5)
               {
                  size = 0.5f / mThreadIDs.Count;
               }

               for(int i = 0; i < mThreadIDs.Count; i++)
               {
                  int threadid = ((int)(mThreadIDs[i]));
                  bottom = lastRegion.mRect.Top;
                  top = bottom - (mGraphRegion.mRect.Height * size);
                  string name = "Thread:" + threadid.ToString();
                  //Frame threadFrame = mCurrentFrame.mThreadFrames[i] as Frame;
                  //string name = mSections[threadFrame.mSamples[0].mSectionID].mName + ":" + threadFrame.mThreadID.ToString() + ":";
                  lastRegion = new FrameDataRegion(name,RectangleF.FromLTRB(left,top,1,bottom),threadid);
                  mOtherThreadRegions.Add(lastRegion);
               }
            }    
            mAllSampleDataRegions.AddRange(mOtherThreadRegions);               
         }
         else if(mViewType == ViewType.PerCPUView)
         {
            mCPURegion.Clear();
            mOtherThreadRegions.Clear();
             
            bottom = mGPUConnectionRegion.mRect.Top;
            top = bottom - (mGraphRegion.mRect.Height * 0.25f);
            mMainThreadRegion = new FrameDataRegion("CPU0",RectangleF.FromLTRB(left,top,1,bottom),mCurrentFrame);
            mAllSampleDataRegions.Add(mMainThreadRegion);
            mCPURegion.Add(mMainThreadRegion);
                              
            float size = 0.10f;
            UIRegion lastRegion = mMainThreadRegion;
            size = 0.5f / 5;
            
            for(int i = 1; i < 6; i++)
            {
               bottom = lastRegion.mRect.Top;
               top = bottom - (mGraphRegion.mRect.Height * size);
               string name = "CPU" + i.ToString();          
               lastRegion = new FrameDataRegion(name,RectangleF.FromLTRB(left,top,1,bottom),null);//threadFrame);
               mCPURegion.Add(lastRegion);
               mOtherThreadRegions.Add(lastRegion);
            }                  
            mAllSampleDataRegions.AddRange(mOtherThreadRegions);
         }
         else if(mViewType == ViewType.PerCoreView)
         {
            mCPURegion.Clear();
            mOtherThreadRegions.Clear();
             
            bottom = mGPUConnectionRegion.mRect.Top;
            top = bottom - (mGraphRegion.mRect.Height * 0.25f);
            mMainThreadRegion = new FrameDataRegion("Core0",RectangleF.FromLTRB(left,top,1,bottom),mCurrentFrame);
            mAllSampleDataRegions.Add(mMainThreadRegion);
            mCPURegion.Add(mMainThreadRegion);
                              
            float size = 0.10f;
            UIRegion lastRegion = mMainThreadRegion;
            size = 0.5f / 2;
            
            for(int i = 1; i < 3; i++)
            {
               bottom = lastRegion.mRect.Top;
               top = bottom - (mGraphRegion.mRect.Height * size);
               string name = "Core" + i.ToString();
               lastRegion = new FrameDataRegion(name,RectangleF.FromLTRB(left,top,1,bottom),null);//threadFrame);
               mCPURegion.Add(lastRegion);
               mOtherThreadRegions.Add(lastRegion);
            }                  
            mAllSampleDataRegions.AddRange(mOtherThreadRegions);
         }

         mMouseTrackerVisual.ClearRegions();
         foreach(UIRegion r in mAllSampleDataRegions)
         {         
            mMouseTrackerVisual.AddRegion(r);
         }
      }


      private void DrawSampleBars(UIRegion region, Frame frame, bool isGPU, double xOffset)
      {
         xOffset = xOffset * Constants.cUIntScaleToMS;
         double mousepos = mMouseWorldLoc * 10000;

         //float mousey = base.GetWorldLocY(this.Height - mMouseY);// / mDomain;// -40;// FindForm the offset
         //float asdf3 = mousey;
         ////this.GetMouseWorldLocation()
         //int ppx;
         //int ppy;
         //float asdf =  (mMouseY / (float)this.Height) * this.mDomain * this.mZoomY + this.mYOffset;
         //base.GetPixel(GetMouseWorldLocation(), mousey, out ppx, out ppy);

         float mousey = (this.Height - mMouseY) / (float)this.Height;

         //float top1 = region.mRect.Top;
         //float bottom1 = region.mRect.Bottom;
         //if (mMouseY > bottom1 && mMouseY < top1)
         //{
         //   return;
         //}

         BRetiredProfileSample sample;

         /////////////visualize call stack for all cpu samples
         if(mRenderOptions == DataRenderOptions.Solid)
         {
            int barColor = Color.MediumSeaGreen.ToArgb();
            barColor = Color.White.ToArgb();

//            if(frame.mFrameNumber % 2 == 0)
//            {
//               barColor = Color.White.ToArgb();
//            }
//            if(frame.mFrameNumber % 2 == 1)
//            {
//               barColor = Color.Red.ToArgb();
//            }
//            if(frame.mFrameNumber % 3 == 2)
//            {
//               barColor = Color.Green.ToArgb();
//            }

            float top = region.mRect.Top;
            float bottom = region.mRect.Bottom;
//                      ResourceHandle t = frame.SafeGetBufferHandle(ref mSections);
//            for(int i=0; i < frame.mNumSamples; i++)
//            {
//               sample = frame.mSamples[i];
//               if(isGPU == false)
//               {
//                  DrawBox(barColor,top , sample.mCPUStartTime+(float)xOffset, sample.mCPUEndTime+(float)xOffset, bottom ); 
//               }
//               else if(sample.mGPUStartTime != sample.mGPUEndTime)
//               {
//                  DrawBox(barColor,top , sample.mGPUStartTime+(float)xOffset, sample.mGPUEndTime+(float)xOffset, bottom ); 
//               }
//            }
            Frame.Line1D line;
            if(isGPU == false)
            {
               for(int i=0; i < frame.mDutyCycle.mSegmentList.Count; i++)
               {
                  line = frame.mDutyCycle.mSegmentList[i] as Frame.Line1D;
                  DrawBox(barColor,top , line.mStart+(float)xOffset, line.mEnd+(float)xOffset, bottom ); 
               }
            }
            else
            {
               for(int i=0; i < frame.mGpuDutyCycle.mSegmentList.Count; i++)
               {
                  line = frame.mGpuDutyCycle.mSegmentList[i] as Frame.Line1D;
                  DrawBox(barColor,top , line.mStart+(float)xOffset, line.mEnd+(float)xOffset, bottom ); 
               }
            }

         }
         else if(mRenderOptions == DataRenderOptions.Full)
         {
            ResourceHandle h = frame.SafeGetBufferHandle();
            
            float offset;
            float offsetAmount = (400/20f)/mDomain;
            //mousey = mousey / mDomain;

            if((offsetAmount * (frame.mMaxStackHeight+1)) > region.mRect.Height)
            {
               offsetAmount = region.mRect.Height/(frame.mMaxStackHeight+1);
            }

            float visibleTextThreashold = mRange*mZoomX / 8;

            ArrayList lablels = new ArrayList();


            for(int i=0; i < frame.mNumSamples; i++)
            {
               sample = frame.mSamples[i];
               offset = region.mRect.Top + offsetAmount * sample.mLevel;
               float top = offset;
               float bottom = offset + offsetAmount;
               float height = region.mRect.Height;

               if (mousey < bottom && mousey > top)
               if (mousepos > sample.mCPUStartTime + xOffset && mousepos < sample.mCPUEndTime + xOffset)
               {
                  mSampleUnderMouse = sample;
                  //DrawLine(Color.White.ToArgb(), sample.mCPUStartTime + xOffset, top, sample.mCPUStartTime + xOffset, bottom);
                  //DrawLine(Color.White.ToArgb(), sample.mCPUEndTime + xOffset, top, sample.mCPUEndTime + xOffset, bottom);
                  //continue;
               }
               if (mousepos > sample.mCPUStartTime + xOffset && mousepos < sample.mCPUEndTime + xOffset)
               {
                  mousepos = mousepos;
               }

               if(isGPU == false)
               {
                  DrawBox(GetColor(sample.mSectionID),top , sample.mCPUStartTime+(float)xOffset, sample.mCPUEndTime+(float)xOffset, bottom ); 
                  if((sample.mCPUEndTime - sample.mCPUStartTime) > visibleTextThreashold)
                  {  
                     lablels.Add(sample);
                  }
               }
               else if(sample.mGPUStartTime != sample.mGPUEndTime)
               {
                  DrawBox(GetColor(sample.mSectionID),top , sample.mGPUStartTime+(float)xOffset, sample.mGPUEndTime+(float)xOffset, bottom );
                  if((sample.mGPUEndTime - sample.mGPUStartTime) > visibleTextThreashold)
                  {  
                     lablels.Add(sample);
                  }
               }

               string text = mSections[sample.mSectionID].mName;
               if (text.Contains("BTerrainIOLoader"))
               {
                  lablels.Add(sample);

               }
            }
            FlushBoxes();

            //Label boxes
            float textHeight = 20 / mDomain;
            int px; int py;
            float startpos=0;
            float endpos=0;
            foreach(BRetiredProfileSample s in lablels)
            {
               sample = s;
               float top = region.mRect.Top + offsetAmount * sample.mLevel;
               string text = mSections[sample.mSectionID].mName;
               bool bTextVisible = false;

               if(isGPU == false)
               {
                  startpos = sample.mCPUStartTime+(float)xOffset;
                  endpos = sample.mCPUEndTime+(float)xOffset;
               }
               else
               {
                  startpos = sample.mGPUStartTime+(float)xOffset;
                  endpos = sample.mGPUEndTime+(float)xOffset;
               }
               if(startpos < mXOffset)
               {
                  startpos = mXOffset;
               }
               if((GetPixel(startpos,top+textHeight,out px, out py)) && (endpos > mXOffset + visibleTextThreashold/2))
               {
                  bTextVisible = true;
               }

               if(bTextVisible)
               {
                  Color c;
                  c = Color.FromArgb(GetColor(sample.mSectionID));
                  //if(c.GetBrightness() < .5f) 
                  if(c.R*c.R*0.7+c.G*c.G*1.4+c.B*c.B*0.5 > 255*255)
                  {
                     DrawText(px,py,text,Color.Black.ToArgb());
                  }
                  else
                  {
                     DrawText(px,py,text,Color.White.ToArgb());
                  }
               }


            }           
         }
         FlushBoxes();
      }



      private void DrawSampleLines(UIRegion region, UIRegion connectregion, Frame frame, bool isGPU, double xOffset)
      {

         xOffset = xOffset * Constants.cUIntScaleToMS;
         BRetiredProfileSample sample;

         double mousepos = mMouseWorldLoc * 10000;

         ///////Draw Sample Lines
         int white = System.Drawing.Color.White.ToArgb();
         int c = white;

         float top = region.mRect.Top;
         float bottom = region.mRect.Bottom;

         BRetiredProfileSample selectedSample = null;
         if(mRenderOptions == DataRenderOptions.Solid)
         {
         }
         else if(mRenderOptions == DataRenderOptions.Full)
         {
            ResourceHandle h = frame.SafeGetBufferHandle();

            for(int i=0; i < frame.mNumSamples; i++)
            {
               sample = frame.mSamples[i];
               c++;
               if(isGPU == false)
               {               
                  DrawLine(c, sample.mCPUStartTime+xOffset,top, sample.mCPUStartTime+xOffset,bottom);
                  DrawLine(c, sample.mCPUEndTime  +xOffset,top, sample.mCPUEndTime  +xOffset,bottom);
               }
               else if(sample.mGPUStartTime != sample.mGPUEndTime)
               {
                  DrawLine(c, sample.mGPUStartTime+xOffset,top, sample.mGPUStartTime+xOffset,bottom);
                  DrawLine(c, sample.mGPUEndTime  +xOffset,top, sample.mGPUEndTime  +xOffset,bottom);

                  if(connectregion != null)
                  {                  
                     DrawLine(c, sample.mCPUStartTime+xOffset,connectregion.mRect.Top, sample.mGPUStartTime+xOffset,connectregion.mRect.Bottom);
                     DrawLine(c, sample.mCPUEndTime  +xOffset,connectregion.mRect.Top, sample.mGPUEndTime  +xOffset,connectregion.mRect.Bottom);  
                  }

               }

               //draw lines for any selected frame
               if (mousepos > sample.mCPUStartTime + xOffset && mousepos < sample.mCPUEndTime + xOffset)
               {
                  selectedSample = sample;
                  //DrawLine(Color.White.ToArgb(), sample.mCPUStartTime + xOffset, top, sample.mCPUStartTime + xOffset, bottom);
                  //DrawLine(Color.White.ToArgb(), sample.mCPUEndTime + xOffset, top, sample.mCPUEndTime + xOffset, bottom);
               }
            }
            if (selectedSample != null)
            {
               //sample = selectedSample;
               //DrawLine(Color.White.ToArgb(), sample.mCPUStartTime + xOffset, top, sample.mCPUStartTime + xOffset, bottom);
               //DrawLine(Color.White.ToArgb(), sample.mCPUEndTime + xOffset, top, sample.mCPUEndTime + xOffset, bottom);
            }

         }
         FlushLines();
      }

      private void DrawFrameData(Frame frame, double startTime)
      {


         DrawSampleLines(this.mMainThreadRegion,null,frame,false,frame.mStartTime - startTime);
         DrawSampleBars(this.mMainThreadRegion,frame,false,frame.mStartTime - startTime);

         if (frame.hasGPUSamples() == true)
         {
            DrawSampleLines(this.mGPURegion, this.mGPUConnectionRegion, frame, true, frame.mStartTime - startTime);
            DrawSampleBars(this.mGPURegion, frame, true, frame.mStartTime - startTime);
         }
         foreach (Frame f in frame.mThreadFrames)
         {
            if (f.hasGPUSamples() == true)
            {
               DrawSampleLines(this.mGPURegion, this.mGPUConnectionRegion, f, true, f.mStartTime - startTime);
               DrawSampleBars(this.mGPURegion, f, true, f.mStartTime - startTime);
            }
         }

         if(mViewType == ViewType.ThreadView)
         {   
            foreach(Frame f in frame.mThreadFrames)
            {
               foreach(FrameDataRegion fregion in mOtherThreadRegions)
               {
                  if(f.mThreadID == fregion.mThreadId)
                  {                    
                     DrawSampleLines(fregion,null,f,false,f.mStartTime - startTime);
                     DrawSampleBars(fregion,f,false,f.mStartTime - startTime);



                     break;
                  }
               }
            }       
//            foreach(FrameDataRegion fregion in mOtherThreadRegions)
//            {
//               ResourceHandle t = fregion.mFrame.SafeGetBufferHandle();
//
//               DrawSampleLines(fregion,null,fregion.mFrame,false,fregion.mFrame.mStartTime - startTime);
//               DrawSampleBars(fregion,fregion.mFrame,false,fregion.mFrame.mStartTime - startTime);
//            }
         }
         else if(mViewType == ViewType.PerCPUView) 
         {           
            foreach(Frame f in frame.mThreadFrames)
            {
               if (f.mCPUID != -1)
               {

                  FrameDataRegion fregion = mCPURegion[f.mCPUID] as FrameDataRegion;
                  //fregion.mName = f.mFrameNumber.ToString();//debug info
                  DrawSampleLines(fregion, null, f, false, f.mStartTime - startTime);
                  DrawSampleBars(fregion, f, false, f.mStartTime - startTime);
               }
            }
         }
         else if(mViewType == ViewType.PerCoreView)
         {
            foreach(Frame f in frame.mThreadFrames)
            {
               int coreID = (int)(Math.Floor(f.mCPUID / 2f));
               if (coreID >= 0)
               {
                  FrameDataRegion fregion = mCPURegion[coreID] as FrameDataRegion;
                  DrawSampleLines(fregion, null, f, false, f.mStartTime - startTime);
                  DrawSampleBars(fregion, f, false, f.mStartTime - startTime);
               }
            }
         }
      }


      ArrayList mOtherFrames = new ArrayList();

      private void DetermineFramesToDraw(double startTime, double endTime)
      {
         Frame mainFrame = mCurrentFrame;
         
         mOtherFrames.Clear();

         Frame previous = mainFrame.mLastFrame;         
         while((previous != null) 
            && (startTime <= (previous.mStartTime + previous.mCPUEndtime/10000000)) )
            //&& (previous.mStartTime < endTime) )
         {
            mOtherFrames.Add(previous);
            previous = previous.mLastFrame;                           
         }
         Frame nextFrame = mainFrame.mNextFrame;
         while((nextFrame != null) 
            && (endTime >= nextFrame.mStartTime) )
            //&& (startTime < nextFrame.mStartTime))
         {
            mOtherFrames.Add(nextFrame);
            nextFrame = nextFrame.mNextFrame;                           
         }

      }

      private int DrawScale()
      {
         #region YUCK
         //draw the scale ... the crappy old way
         RectangleF graphRect = new RectangleF(0,0,mRange*mZoomX,mDomain*mZoomY);
         SetView(graphRect );         
         int timeScaleUnits = (int)Math.Ceiling(mRange*mZoomX / (40 * Constants.cUIntScaletoSeconds));
         RectangleF scale = new RectangleF(0,50,mRange*mZoomX,50);
         //RectangleF scale2 = new RectangleF(0,950,mRange*mZoomX,50);
         int scaleColor = System.Drawing.Color.White.ToArgb();
         DrawScale(scaleColor,scale,0, mOneMS * timeScaleUnits,true,true,true);      
         //DrawScale(scaleColor,scale2,0,mOneMS * timeScaleUnits,true,true,false);      
         FlushAll();
         float ypos = this.Height - 0.25f * (mBottomScaleRegion.mRect.Top + mBottomScaleRegion.mRect.Bottom*3)* this.Height;
         DrawText(0, ypos, "Tick Scale:" + timeScaleUnits + " ms", Color.White.ToArgb(), Color.Black.ToArgb(),false,false,1);
         return timeScaleUnits;
         #endregion
      }

      double mMouseWorldLoc;

      public override void Draw()
      {

//         if(mCurrentFrame == null || mCurrentFrame.mSamples == null)
//         {
//            SetupDefaultRegions();
//
//            RectangleF uiRect = new RectangleF(0,0,1,1);
//            SetView(uiRect );
//
//            DrawBox(Color.White.ToArgb(),mTopScaleRegion.mRect);
//            DrawBox(Color.White.ToArgb(),mBottomScaleRegion.mRect);
//            //DrawBox(Color.White.ToArgb(),mGraphRegion.mRect);
//
//            int shiftAmmount = 10; //change the background colors
//            DrawBox(System.Drawing.Color.DarkSlateGray.ToArgb() + shiftAmmount,mGPURegion.mRect);
//            DrawBox(0x393939,mGPUConnectionRegion.mRect);
//            DrawBox(System.Drawing.Color.DarkSlateGray.ToArgb() - shiftAmmount,mMainThreadRegion.mRect);
//
//            FlushAll();
//
//            return;
//         }
         if(mCurrentFrame == null || mCurrentFrame.mSamples == null)
         {
            return;
         }

         SetupRegions();
         int xscale = Constants.cUIntScaleToMS;
         DetermineFramesToDraw(mCurrentFrame.mStartTime + mXOffset/xscale, mCurrentFrame.mStartTime + mRange*mZoomX/xscale + mXOffset/xscale);         

         try
         {
            mYOffset = 0; //always 0

            //background and scales
            RectangleF uiRect = new RectangleF(0,0,1,1);
            SetView(uiRect );

            DrawBox(Color.White.ToArgb(),mTopScaleRegion.mRect);
            DrawBox(Color.White.ToArgb(),mBottomScaleRegion.mRect);
            //DrawBox(Color.White.ToArgb(),mGraphRegion.mRect);

            int shiftAmmount = 10; //change the background colors
            DrawBox(System.Drawing.Color.DarkSlateGray.ToArgb() + shiftAmmount,mGPURegion.mRect);
            DrawBox(0x393939,mGPUConnectionRegion.mRect);
            DrawBox(System.Drawing.Color.DarkSlateGray.ToArgb() - shiftAmmount,mMainThreadRegion.mRect);

            bool bflipflop = false;
            foreach(FrameDataRegion fregion in mOtherThreadRegions)
            {
               DrawBox(System.Drawing.Color.DarkSlateGray.ToArgb() + ((bflipflop)?-shiftAmmount:shiftAmmount) ,fregion.mRect);
               bflipflop = !bflipflop;
            }

            foreach(FrameDataRegion fregion in mAllSampleDataRegions)
            {
               DrawLine(Color.Black.ToArgb(),0,fregion.mRect.Top,1,fregion.mRect.Top);
            }

            FlushAll();

            DrawScale();
            
            RectangleF worldRect = new RectangleF(mXOffset,mYOffset,mRange*mZoomX,1*mZoomY);
            SetView(worldRect );            
    
            double startTime = mCurrentFrame.mStartTime;
            double startms = this.mXOffset / mOneMS;
            double endms = startms + (this.mRange / mOneMS) * this.mZoomX;
            double mousems = GetMouseWorldLocation() / mOneMS;
            mMouseWorldLoc = mousems;

            //indicate the current frame
            DrawBox(System.Drawing.Color.Black.ToArgb(),  
               mTopScaleRegion.mRect.Top,
               0f,
               (float)((mCurrentFrame.mNextFrameStartTime - startTime) * Constants.cUIntScaleToMS),
               mTopScaleRegion.mRect.Bottom);

            mSampleUnderMouse = null;  //this is searched for during drawing

            DrawFrameData(mCurrentFrame,startTime);

            foreach(Frame f in mOtherFrames)
            {               
               DrawFrameData(f,startTime);
            }

            mMouseTrackerVisual.Draw(this.mMouseDown,this.mMouseX,this.mMouseY);

            //info text
            DrawText(
               0,this.Height - 20,
               String.Format("Frame:{0} Samples:{1} Scale:{2:.###}ms",mCurrentFrame.mFrameNumber, mCurrentFrame.TotalSamples(),endms - startms)
               + String.Format("   From {0:.###}ms to {1:.###}ms  Mouse: {2:.###}ms", startms,endms,mousems), 
               System.Drawing.Color.White.ToArgb());

            //selected sample text
            if (mSampleUnderMouse != null)
            {
               float time = (mSampleUnderMouse.mCPUEndTime - mSampleUnderMouse.mCPUStartTime) / mOneMS;
               string sample = String.Format("{0}   {1:.###}", mSections[mSampleUnderMouse.mSectionID].mName, time);
               DrawText(650, this.Height - 20, sample, System.Drawing.Color.Goldenrod.ToArgb());

            }

            int framelabeloffset = -(int)(this.Width * (mXOffset/(mZoomX * mRange)));//*mZoomX);

            DrawBigText(framelabeloffset ,2,"Frame:"+mCurrentFrame.mFrameNumber ,Color.White.ToArgb());

            foreach(FrameDataRegion fregion in mAllSampleDataRegions)
            {
               //DrawBox()
               float ypos;
               if(fregion.mRect.Height > .05)
               {
                  //ypos = this.Height - 0.5f * (fregion.mRect.Top + fregion.mRect.Bottom)* this.Height;
                  ypos = this.Height - 0.25f * (fregion.mRect.Top + fregion.mRect.Bottom*3)* this.Height;
                  DrawBigText(
                     0,ypos,
                     fregion.mName,
                     System.Drawing.Color.White.ToArgb());     
               }
               else
               {
                  ypos = this.Height - 0.25f * (fregion.mRect.Top + fregion.mRect.Bottom*3)* this.Height;
                  DrawText(
                     0,ypos,
                     fregion.mName,
                     System.Drawing.Color.White.ToArgb());      
               }

      
            }

            if (mShowTextScale)
            {
//               DrawText(mMouseX, mMouseY, (mLastDisplacement).ToString() + "ms", Color.White.ToArgb());
               DrawText(mMouseX, mMouseY, String.Format("   {0:F3} ms", Math.Abs(mLastDisplacement) / 10000.0), Color.White.ToArgb());

               mMouseTrackerVisual.Draw(this.mMouseDown, this.mMouseX2, this.mMouseY);

            }


//            //         HighlightStack(mSelectedCallStackGPU);
//            //         HighlightStack(mSelectedCallStackCPU);
//
//            float left = this.Width * 0.7f;
//            float right = this.Width * 1f;
//            RectangleF thisWindow = new RectangleF(left,0,right-left,this.Height / 2f);
//            if(mMarker != null)
//            {
//               UIRegion region = mMarker.GetRegion();
//               SetView(worldRect );         
//               DrawLine(System.Drawing.Color.Red.ToArgb(), mMarker.GetMarkerTime(),region.mRect.Top, mMarker.GetMarkerTime(),region.mRect.Bottom);
//               FlushLines();               
////               if(true)
////               {
////                  float left = this.Width * 0.7f;
////                  float right = this.Width * 1f;
////                  RectangleF thisWindow = new RectangleF(left,0,right-left,this.Height / 2f);
////
////                  ArrayList samples;
////                  if(mGPURegion == region)
////                  {
////                     samples = FindSamples(mMarker.GetMarkerTime(),SampleType.GPU);
////                  }
////                  else
////                  {
////                     samples = FindSamples(mMarker.GetMarkerTime(),SampleType.CPU);
////                  }
////
////                  DrawStackInformation(mSelectedCallStackCPU, SampleType.CPU, mMarker.GetMarkerTime(), thisWindow);
////               }
//            }

         }
         catch(System.Exception ex)
         {
            ErrorHandler.Error(ex.ToString());
         }
      }


      #region OldDraw
      /*
      public void Draw2()
      {
         BRetiredProfileSample sample;


         mCurrentFrameHandle = mCurrentFrame.SafeGetBufferHandle();

         try
         {
            if(mCurrentFrame == null || mCurrentFrame.mSamples == null)
               return;

            //mXOffset = (float)mCurrentFrame.getCPUStartTime() + mXScrollOffset * this.mRange;
            mYOffset = 0; //always 0

            RectangleF graphRect = new RectangleF(0,-300,mRange*mZoomX,mDomain*mZoomY);
            SetView(graphRect );         

            int timeScaleUnits = 1; //ms
            RectangleF scale = new RectangleF(0,0,mRange*mZoomX,50);
            RectangleF scale2 = new RectangleF(0,950,mRange*mZoomX,50);
            int scaleColor = System.Drawing.Color.White.ToArgb();
            DrawScale(scaleColor,scale,0,mOneMS * timeScaleUnits,true,true,true);      
            DrawScale(scaleColor,scale2,0,mOneMS * timeScaleUnits,true,true,false);      
            DrawBox(System.Drawing.Color.DarkSlateGray.ToArgb(),0,0,mRange*mZoomX,450);
            DrawBox(System.Drawing.Color.DarkSlateGray.ToArgb(),550,0,mRange*mZoomX,1000);
         
            DrawBox(0x393939,450,0,mRange*mZoomX,550);

            FlushAll();

            int white = System.Drawing.Color.White.ToArgb();
     
            RectangleF worldRect = new RectangleF(mXOffset,mYOffset,mRange*mZoomX,mDomain*mZoomY);
            SetView(worldRect );         
  
            ///////Draw Sample Lines
            int c = white;
            lock(this)
            {
               for(int i=0; i < mCurrentFrame.mNumSamples; i++)
               {
                  sample = mCurrentFrame.mSamples[i];
                  c++;
                  DrawLine(c, sample.mCPUStartTime,50, sample.mCPUStartTime,450);
                  DrawLine(c, sample.mCPUEndTime  ,50, sample.mCPUEndTime  ,450);

                  if(sample.mGPUStartTime != sample.mGPUEndTime)
                  {         
                     DrawLine(c, sample.mCPUStartTime,450, sample.mGPUStartTime,550);
                     DrawLine(c, sample.mCPUEndTime  ,450, sample.mGPUEndTime  ,550);  
                     DrawLine(c, sample.mGPUStartTime,550, sample.mGPUStartTime,950);
                     DrawLine(c, sample.mGPUEndTime  ,550, sample.mGPUEndTime  ,950);
                  }            
               }
            }
            FlushLines();

            /////////////visualize call stack for all cpu samples
            float offset;
            float offsetAmount = 400/20f;
            for(int i=0; i < mCurrentFrame.mNumSamples; i++)
            {
               sample = mCurrentFrame.mSamples[i];
               offset = 50 + offsetAmount * sample.mLevel;
               float top = offset;
               float bottom = offset + offsetAmount;

               DrawBox(GetColor(sample.mSectionID),top , sample.mCPUStartTime, sample.mCPUEndTime, bottom ); 
      

               if(sample.mGPUStartTime != sample.mGPUEndTime)
               {             
                  DrawBox(GetColor(sample.mSectionID),1000-top , sample.mGPUStartTime, sample.mGPUEndTime, 1000-bottom );
               }
         
            }
            FlushBoxes();

//            //draw threads ...
//            foreach(Frame f in mCurrentFrame.mThreadFrames)
//            {
//               for(int i=0; i < f.mNumSamples; i++)
//               {
//                  sample = f.mSamples[i];
//                  offset = 50 + offsetAmount * sample.mLevel;
//                  float top = offset;
//                  float bottom = offset + offsetAmount;
//                  DrawBox(GetColor(sample.mSectionID),top , sample.mCPUStartTime, sample.mCPUEndTime, bottom ); 
//               }
//               FlushBoxes();
//            }


            mMouseTrackerVisual.Draw(this.mMouseDown,this.mMouseX,this.mMouseY);

            //info text
            double startms = this.mXOffset / mOneMS;
            double endms = startms + ( this.mRange / mOneMS) * this.mZoomX;
            double mousems = GetMouseWorldLocation() / mOneMS;
            DrawText(
               0,0,
               String.Format("Frame:{0} Samples:{1} Scale:{2:.###}ms",mCurrentFrame.mFrameNumber, mCurrentFrame.mNumSamples,endms - startms)
               + String.Format("   From {0:.###}ms to {1:.###}ms  Mouse: {2:.###}ms", startms,endms,mousems), 
               System.Drawing.Color.White.ToArgb());

            //         HighlightStack(mSelectedCallStackGPU);
            //         HighlightStack(mSelectedCallStackCPU);

            float left = this.Width * 0.7f;
            float right = this.Width * 1f;
            RectangleF thisWindow = new RectangleF(left,0,right-left,this.Height / 2f);

            if(mCPUMarker != null)
            {
               SetView(worldRect );         
               DrawLine(System.Drawing.Color.Red.ToArgb(), mCPUMarker.GetMarkerTime(),50, mCPUMarker.GetMarkerTime(),500);
               FlushLines();
               DrawStackInformation(mSelectedCallStackCPU, SampleType.CPU, mCPUMarker.GetMarkerTime(), thisWindow);
            }         
            if(mGPUMarker != null)
            {
               thisWindow.Offset(0,this.Height / 2f);
               SetView(worldRect );         
               DrawLine(System.Drawing.Color.Red.ToArgb(), mGPUMarker.GetMarkerTime(),500, mGPUMarker.GetMarkerTime(),1000);
               FlushLines();
               DrawStackInformation(mSelectedCallStackGPU, SampleType.GPU, mGPUMarker.GetMarkerTime(), thisWindow);
            }
         }
         catch(System.Exception ex)
         {
            ErrorHandler.Error(ex.ToString());
            //todo: log this error
            //MessageBox.Show(this,"Error Drawing Frame View: " + ex.ToString());
         }

         //highligt connections to gpu on mousedown
      }
      */
      #endregion
      #region OLD_STACK_VIEW
      public void HighlightStack(ArrayList callStack)
      {
         RectangleF worldRect = new RectangleF(mXOffset,mYOffset,mRange*mZoomX,mDomain*mZoomY);

         SetView(worldRect );
         BRetiredProfileSample sample;
         foreach(BRetiredProfileSample selectedSample in callStack)
         {
            sample = selectedSample;
            int sampleColor = GetColor(sample.mSectionID);

            int color = System.Drawing.Color.White.ToArgb();
            if(sample.mGPUStartTime != sample.mGPUEndTime)
            {         
               DrawLine(color, sample.mCPUStartTime,450, sample.mGPUStartTime,550);
               DrawLine(color, sample.mCPUEndTime  ,450, sample.mGPUEndTime  ,550);  
               DrawLine(color, sample.mGPUStartTime,550, sample.mGPUStartTime,950);
               DrawLine(color, sample.mGPUEndTime  ,550, sample.mGPUEndTime  ,950);
                  
               DrawLine(sampleColor, sample.mGPUStartTime,650, sample.mGPUStartTime,950);
               DrawLine(sampleColor, sample.mGPUEndTime  ,650, sample.mGPUEndTime  ,950);
            } 
         }
         FlushLines();
      }

      protected void DrawStackInformation(ArrayList callStack, SampleType type, float time ,RectangleF rect )//float x1, float x2)
      {
         //this offset is a bit of a hack since text does not follow the same coordinates...
         float textOffset = (rect.Height * 2) - rect.Bottom;
         BRetiredProfileSample sample;

         this.SetViewToPixels();
         DrawBox(System.Drawing.Color.Black.ToArgb(),rect);
         DrawBox(System.Drawing.Color.Blue.ToArgb(),rect.Bottom-12,rect.Left,rect.Right,rect.Bottom);
         DrawBox(System.Drawing.Color.White.ToArgb(),rect.Bottom-12,rect.Right - 12,rect.Right,rect.Bottom);
         FlushBoxes();

         if(SampleType.GPU == type)
         {
            mCloseGPUStack.mRect = new RectangleF(rect.Right - 12,textOffset,  12, 12);
            string title = String.Format("GPU Stack@{0:0.0000}ms",time / mOneMS);
            DrawText(rect.Left,textOffset,title,System.Drawing.Color.White.ToArgb());
         }
         else
         {           
            mCloseCPUStack.mRect = new RectangleF(rect.Right - 12,textOffset,  12, 12);
            string title = String.Format("CPU Stack@{0:0.0000}ms",time / mOneMS);
            DrawText(rect.Left,textOffset,title,System.Drawing.Color.White.ToArgb()); 
         }
         foreach(BRetiredProfileSample selectedSample in callStack)
         {
            sample = selectedSample;
            int sampleColor = GetColor(sample.mSectionID);

            double cputime = (sample.mCPUEndTime - sample.mCPUStartTime) / mOneMS;
            double gputime = (sample.mGPUEndTime - sample.mGPUStartTime) / mOneMS;
            string stack;
            #region Show all times
//               if(gputime != 0)
//               {
//                  stack = String.Format("C:{0:0.0000} G:{1:0.0000} {2}",cputime ,gputime, mSections[sample.mSectionID].mName);
//               }
//               else
//               {
//                  stack = String.Format("C:{0:0.0000}              {1}",cputime, mSections[sample.mSectionID].mName);
//               }
            #endregion
 

            string sectionName;

            if(sample.mSectionID > mNumSections)
            {
               ErrorHandler.Error("Invalid Section ID: " + sample.ToString());
               sectionName = String.Format("Error: {1} is an invalid ID!", sample.mSectionID);
            }
            else
            {
               sectionName = mSections[sample.mSectionID].mName;
            }

            if(SampleType.CPU == type)
            {
               stack = String.Format("{0:0.0000} {1}",cputime, sectionName);
            }
            else
            {
               stack = String.Format("{0:0.0000} {1}",gputime, sectionName);
            }

            DrawText(
               rect.Left, 
               12 * sample.mLevel + 12 + textOffset,
               stack,
               sampleColor);
         }
      
      }

      #endregion
      
      
      #endregion

		#region Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
         // 
         // ProfilerFrameView
         // 
         this.BackColor = System.Drawing.Color.DarkSlateGray;
         this.Name = "ProfilerFrameView";
         this.Size = new System.Drawing.Size(664, 296);
         this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.ProfilerFrameView_MouseUp);
         this.Paint += new System.Windows.Forms.PaintEventHandler(this.ProfilerFrameView_Paint);
         this.MouseMove += new System.Windows.Forms.MouseEventHandler(this.ProfilerFrameView_MouseMove);
         this.MouseDown += new System.Windows.Forms.MouseEventHandler(this.ProfilerFrameView_MouseDown);

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
      private void ProfilerFrameView_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
      {
         mShowTextScale = false;

         if (mMouseDown == true)
         {
            float newLocation = GetMouseWorldLocation(e.X);
            float displacement = mMouseXWorldLocation - newLocation;

            mLastDisplacement = displacement;

            mShowTextScale = true;
            UpdateIfNeeded();

            mMouseX2 = e.X;

         }
         else if (mMiddleMouseDown == true)
         {
            float newLocation = GetMouseWorldLocation(e.X);
            float displacement = mMouseXWorldLocation - newLocation;
            this.mXOffset = mLastOffsetX + displacement;

            //this.ParentForm.Text = displacement.ToString();

            UpdateIfNeeded();
         }   
         else 
         {
            mMouseX = e.X;
            mMouseY = e.Y;         

            UpdateIfNeeded();
            if (mMouseDown == true)
               OnGraphClick();
         }
      }
      bool mShowTextScale = true;
      float mLastDisplacement = 0;
      //scrolling vars
      int mMouseDownX = -1;
      int mMouseDownY = -1;
      float mMouseXWorldLocation = -1;
      float mLastOffsetX = -1;
      bool mMiddleMouseDown = false;

      int mMouseX2 = -1;

      private void ProfilerFrameView_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
      {
         mMouseDownX = e.X;
         mMouseDownY = e.Y;
         mLastOffsetX = mXOffset;
         mMouseXWorldLocation = GetMouseWorldLocation(mMouseDownX);

         if (e.Button == MouseButtons.Middle)
         {
            Cursor.Current = Cursors.NoMove2D;

            mMiddleMouseDown = true;
         }
         else
         {
            mMouseDown = true;
            //OnGraphClick();
            //UpdateIfNeeded();
            //ButtonClickTest(e.X, e.Y);
         }
      }
      private void ProfilerFrameView_MouseUp(object sender, System.Windows.Forms.MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Middle)
         {
            Cursor.Current = Cursors.Default;
            mMiddleMouseDown = false;
         }
         else
         {
            mMouseDown = false;
            UpdateIfNeeded();
         }
      }

      private void OnGraphClick()
      {
         float xWorldPosition;
         UIRegion region = mMouseTrackerVisual.GetSelectedRegion(this.mMouseX, this.mMouseY);
         if(region == null) 
            return;
         xWorldPosition = GetMouseWorldLocation();

         //mSelectedCallStackGPU = FindSamples(xWorldPosition,SampleType.GPU);
         if(region is FrameDataRegion)
         {         
            mMarker = new TimeLineMarker(xWorldPosition,region);
         }
//         if(region.Equals(mGPURegion))
//         {
//            mSelectedCallStackGPU = FindSamples(xWorldPosition,SampleType.GPU);
//            mGPUMarker = new TimeLineMarker(xWorldPosition);
//         }
//         else if(region.Equals(mCPURegion))
//         {
//            mSelectedCallStackCPU = FindSamples(xWorldPosition,SampleType.CPU);
//            mCPUMarker = new TimeLineMarker(xWorldPosition);
//         }
      }
      public float GetMouseWorldLocation()
      {
         return (mMouseX / (float)this.Width) * this.mRange * this.mZoomX + this.mXOffset;
      }
      public float GetMouseWorldLocation(int screenX)
      {
         return (screenX / (float)this.Width) * this.mRange * this.mZoomX + this.mLastOffsetX;
      }


      public enum SampleType
      {
         CPU,
         GPU
      }

      private ArrayList FindSamples(float time, SampleType type)
      {
         ArrayList selected = new ArrayList();

         if(mCurrentFrame == null)
         {
            return selected;
         }
         BRetiredProfileSample sample;
         for(int i=0; i < mCurrentFrame.mNumSamples; i++)
         {
            sample = mCurrentFrame.mSamples[i];
            if(type == SampleType.CPU)
            {
               if((sample.mCPUStartTime <= time) && (sample.mCPUEndTime >= time))
               {
                  selected.Add(sample);
               }
            }
            else if(type == SampleType.GPU)
            {
               if((sample.mGPUStartTime <= time) && (sample.mGPUEndTime >= time))
               {
                  selected.Add(sample);
               }
            }
         }
         return selected;
      }

      private void ProfilerFrameView_MouseWheel(object sender, MouseEventArgs e)
      {
         
      }



      private void mCloseGPUStack_Clicked(UIRegion sender)
      {
         //mGPUMarker = null;
      }

      private void mCloseCPUStack_Clicked(UIRegion sender)
      {
         //mCPUMarker = null;
      }

      private void ProfilerFrameView_Paint(object sender, System.Windows.Forms.PaintEventArgs e)
      {
         this.UpdateIfNeeded();

      }

      #endregion
   }

   public class TimeLineMarker
   {
      float mPosition;
      UIRegion mRegion;
      public TimeLineMarker(float position, UIRegion region)
      {
         mPosition = position;
         mRegion = region;
      }
      public TimeLineMarker(float position)
      {
         mPosition = position;
      }
      public float GetMarkerTime()
      {
         return mPosition;
      }
      public UIRegion GetRegion()
      {
         return mRegion;
      }
   }

}

