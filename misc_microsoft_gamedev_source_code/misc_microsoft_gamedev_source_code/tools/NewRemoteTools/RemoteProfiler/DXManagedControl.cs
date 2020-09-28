using System;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Windows.Forms;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;


using RemoteTools;

namespace EnsembleStudios.RemoteGameDebugger
{
	public class DXManagedControl : System.Windows.Forms.UserControl
	{
      #region FormsStuff
		private System.ComponentModel.Container components = null;
		public DXManagedControl()
		{
			//InitializeComponent();
         //InitializeGraphics();
		}

		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

      #endregion

      //This is the one method to overload and include all of your drawing code in.
      virtual public void Draw()
      {

      }
      //State
      private bool mPaused = false;

      //Settings
      public int mBackgroundColor = System.Drawing.Color.Blue.ToArgb();
      public bool mbDrawAALines = true;

      #region DirectXJunk

      Device mDevice = null; 
      
      //Data
      ArrayList mBoxVertexArray = new ArrayList(); 
      ArrayList mLineVertexArray = new ArrayList(); 
      Microsoft.DirectX.Direct3D.Font mBasicFont = null;
      Microsoft.DirectX.Direct3D.Font mGoodFont = null;

      //Engine Performance variables
      //LineBuffer
      private bool mUseLineBuffer = false;
      private CustomVertex.PositionColored[] mLineBuffer;
      private int mLineBufferCount;
      private int mLineBufferSize;
      //BoxBuffer
      private bool mUseBoxBuffer = false;
      private CustomVertex.PositionColored[] mBoxBuffer;
      private int mBoxBufferCount;
      private int mBoxBufferSize;

      // Interop to call get device caps
      private const int LogPixelsY = 90;
      [System.Runtime.InteropServices.DllImport("gdi32.dll")]
      private static extern int GetDeviceCaps(IntPtr hdc, int cap);

      //font variables
      protected void InitFont()
      {
         string fontName = "Arial";
         const int FontSize = 10;
         int height;
         int logPixels;
         // Initialize all of the fonts
         using(System.Drawing.Graphics g = System.Drawing.Graphics.FromHwnd(this.Handle))
         {
            System.IntPtr dc = g.GetHdc();
            logPixels = GetDeviceCaps(dc, LogPixelsY);
            g.ReleaseHdc(dc);
         }
         height = -FontSize * logPixels / 72;
         mBasicFont = new Microsoft.DirectX.Direct3D.Font(mDevice, height, 0, FontWeight.Bold,
            1, false, CharacterSet.Default, Precision.Default, FontQuality.Default, 
            PitchAndFamily.DefaultPitch | PitchAndFamily.FamilyDoNotCare, fontName);

         height = -14 * logPixels / 72;
         mGoodFont = new Microsoft.DirectX.Direct3D.Font(mDevice, height, 0, FontWeight.Bold,
            1, false, CharacterSet.Default, Precision.Default, FontQuality.Default, 
            PitchAndFamily.DefaultPitch | PitchAndFamily.FamilyDoNotCare, fontName);

      }
      //This always uses the same font, to use more a font manager would be needed for the engine..
      public void DrawText(float x, float y, string text, int color)
      { 
         //normalize color since text is picky
         System.Drawing.Color a = System.Drawing.Color.FromArgb(color);
         System.Drawing.Color c = System.Drawing.Color.FromArgb(a.R,a.G,a.B);
           
         mBasicFont.DrawText(null, text, new System.Drawing.Rectangle((int)x,(int)y, 0, 0), 
            DrawTextFormat.NoClip, c.ToArgb());
      }
      public void DrawBigText(float x, float y, string text, int color)
      { 
         //normalize color since text is picky
         System.Drawing.Color a = System.Drawing.Color.FromArgb(color);
         System.Drawing.Color c = System.Drawing.Color.FromArgb(a.R,a.G,a.B);
           
         mGoodFont.DrawText(null, text, new System.Drawing.Rectangle((int)x,(int)y, 0, 0), 
            DrawTextFormat.NoClip, c.ToArgb());
      }
      public void DrawText(float x, float y, string text, int color, int backColor, bool dontFlush, bool dontSetView, int inflate)
      { 
         if(!dontSetView)
            SetViewToPixels();
         //normalize color since text is picky
         System.Drawing.Color a = System.Drawing.Color.FromArgb(backColor);
         System.Drawing.Color c = System.Drawing.Color.FromArgb(a.R,a.G,a.B);
           
         //System.Drawing.Rectangle rect = mBasicFont.MeasureString(null,text,DrawTextFormat.NoClip,c);

         //System.Drawing.Rectangle rect = new Rectangle(0,0,1,1);
         //mBasicFont.DrawText(null,text,ref rect,DrawTextFormat.NoClip,c);
   
         //int inflate = 2;

         ////this.DrawBox(c.ToArgb(),(int)(inflate+this.Height - y),(int)(-inflate+x),(int)(inflate+rect.Right+x),(int)(-inflate+this.Height - y - rect.Bottom));
         //this.DrawBox(c.ToArgb(),(inflate+this.Height - y),(-inflate+x),(inflate+rect.Right+x),(-inflate+this.Height - y - rect.Bottom));

         if(!dontFlush)
            this.FlushBoxes();
         DrawText(x, y, text, color);
      }      

      protected void Render()
      {
         if (mDevice == null) 
            return;
         //to make rendering a mostly non blocking call I use a fancy variation of lock()
         if(!System.Threading.Monitor.TryEnter(this, 5))
         {
            return;
         }
         else
         {
            try
            {
               if (mPaused)
                  return;

               //Clear the backbuffer to a blue color 
               mDevice.Clear(ClearFlags.Target, mBackgroundColor, 1.0f, 0);
               //Begin the scene
               mDevice.BeginScene();
               // Turn off culling, so we see the front and back of the triangle
               mDevice.RenderState.CullMode = Cull.None;
               // Turn off D3D lighting, since we are providing our own vertex colors
               mDevice.RenderState.Lighting = false;
               mDevice.RenderState.AntiAliasedLineEnable = mbDrawAALines;
               mDevice.RenderState.ZBufferEnable = false;

               Draw();
               FlushAll();

               //End the scene
               mDevice.EndScene();

               try
               {
                  mDevice.Present();
               }
               catch(Microsoft.DirectX.Direct3D.DriverInternalErrorException ex)
               {
                  ErrorHandler.Error(ex.ToString());
                  mPaused = true;
               }
            
               mLastRender = System.DateTime.Now;
            }
            catch(System.Exception ex)
            {
               ErrorHandler.Error(ex.ToString());
            }
            finally 
            {
               System.Threading.Monitor.Exit(this);
            }
         }
      }

      protected override void OnEnabledChanged(EventArgs e)
      {


         base.OnEnabledChanged(e);

         if (this.Enabled == true)
         {
            InitializeGraphics();
            UpdateIfNeeded();
         }
      }

      public bool InitializeGraphics()
      {
         try
         {
            if (this.Enabled == false)
               return false;

            // Now let's setup our D3D stuff
            PresentParameters presentParams = new PresentParameters();
            presentParams.Windowed=true;
            presentParams.SwapEffect = SwapEffect.Discard;
            mDevice = new Device(0, 
               DeviceType.Hardware, 
               this,
               CreateFlags.HardwareVertexProcessing | CreateFlags.FpuPreserve, 
               presentParams);

            
            mDevice.DeviceReset += new System.EventHandler(this.OnResetDevice);
            mDevice.DeviceLost +=new EventHandler(mDevice_DeviceLost);
            mDevice.Disposing +=new EventHandler(mDevice_Disposing);
            // Turn off culling, so we see the front and back of the triangle
            mDevice.RenderState.CullMode = Cull.None;
            // Turn off D3D lighting, since we are providing our own vertex colors
            mDevice.RenderState.Lighting = false;

            mDevice.RenderState.ZBufferEnable = false;

            //init api...
            InitFont();
            InitLineBuffer();
            InitBoxBuffer();
            InitColorTable(cColorTableSize);
            return true;
         }
         catch (DirectXException ex)
         { 
            ErrorHandler.Error(ex.ToString());
            return false; 
         }
      }
      public void DestroyGraphics()
      {
         if (mDevice != null)
            mDevice.Dispose();
         mDevice = null;
      }

      #region DirectXDeviceEvents
      public void OnResetDevice(object sender, EventArgs e)
      {
         lock(this)
         {
            Device dev = (Device)sender;
            dev.RenderState.CullMode = Cull.None;
            dev.RenderState.Lighting = false;
            mPaused = false;
         }  
      }
      //I get this before the app blows up on minimize, but I don't know what to do..
      //we may just need to handle the exception...
      private void mDevice_DeviceLost(object sender, EventArgs e)
      {
         lock(this)
         { 
            mPaused = true;
         }
      }
      private void mDevice_Disposing(object sender, EventArgs e)
      {
         lock(this)
         { 
            mPaused = true;
            //mDevice.Dispose();
            mDevice = null;
         }
      }
      bool mActiveDeviceWasMinimized = false;

      //It is actually a little tricky to keep directx from exploding on minimize / resize
      protected override void OnResize(System.EventArgs e)
      {
         if((this.Width <= 10) || (this.Height <= 10))
         {
            //Keep dx from exploding
            if(mDevice != null)
            {
               mPaused = true;
               mActiveDeviceWasMinimized = true;
               mDevice.Dispose();
               mDevice = null;
            }
            return;
         }

         //this should be made recursive.  Detecting minimized is not strait forward...
         bool parentMinimized = false;
         if(this.ParentForm != null)
         {
            if(this.ParentForm.WindowState == FormWindowState.Minimized)
               parentMinimized = true;
            if(this.ParentForm.ParentForm != null)
            {
               if(this.ParentForm.ParentForm.WindowState == FormWindowState.Minimized)
                  parentMinimized = true;
            }
            if((parentMinimized) && (mDevice != null))
            {
               mActiveDeviceWasMinimized = true;
               mDevice.Dispose();
               mDevice = null;
            }
         }
         if((mActiveDeviceWasMinimized == true) && (parentMinimized == false) && (mDevice == null))
         {
            InitializeGraphics();
            mActiveDeviceWasMinimized = false;
         }
         mPaused = (!this.Visible || parentMinimized);
      }
      #endregion
 
      #endregion

      #region TheDrawingAPI

      #region NotWorking

      //not working...
      protected void SetView(RectangleF world, RectangleF viewPort, RectangleF device)
      {
         float scalex = viewPort.Width / device.Width;
         float scaley = viewPort.Height / device.Height;
         float offsetx = viewPort.Left - device.Left;
         float offsety = viewPort.Top - device.Top;
         float wScalex = world.Width / device.Width;
         float wScaley = world.Height / device.Height;
         float offx = offsetx * wScalex;// * (viewPort.Width / device.Width));// / 2;
         float offy = offsety * wScaley;// * (viewPort.Height / device.Height));// / 2;//- world.Height;
         float midpointx = world.Left + world.Width / 2.0f;// - offx;
         float midpointy = world.Top + world.Height / 2.0f;// - offy;
         mDevice.Transform.World = Matrix.Identity;
         mDevice.Transform.View = Matrix.LookAtLH( new Vector3( midpointx, midpointy,-5.0f ), new Vector3( midpointx, midpointy, 0.0f ), new Vector3( 0.0f, 1.0f, 0.0f ) );
         mDevice.Transform.Projection = Matrix.OrthoLH(world.Width / scalex,world.Height / scaley , 1.0f, 100.0f);
         throw new System.Exception("This does not work...");
      }
      #endregion


      protected RectangleF mTransform;
      //this makes 2d graph drawing much easier/faster since it does all of the scaling in hardware...
      public void SetView(RectangleF world)
      {
         mTransform = world;
         float midpointx = world.Left + world.Width / 2.0f;// - offx;
         float midpointy = world.Top + world.Height / 2.0f;// - offy;
         mDevice.Transform.World = Matrix.Identity;
         mDevice.Transform.View = Matrix.LookAtLH( new Vector3( midpointx, midpointy,-5.0f ), new Vector3( midpointx, midpointy, 0.0f ), new Vector3( 0.0f, 1.0f, 0.0f ) );
         mDevice.Transform.Projection = Matrix.OrthoLH(world.Width ,world.Height , 1.0f, 100.0f);
      }
      public void SetViewToPixels()
      {
         RectangleF pixelRect = new RectangleF(0,0,Width,Height);
         SetView(pixelRect );
      }

      public bool GetPixel(float x, float y, out int px, out int py)
      {
         px=0;py=0;
         if(!mTransform.Contains(x,y))
         {
            return false;
         }

         double nx = (x - mTransform.Left)/  mTransform.Width ;
         double ny = (y - mTransform.Top)/  mTransform.Height ;


         px = (int)(this.Width*nx);
         py = (int)(this.Height*(1-ny));

         //this.Height

         return true;
      }
      public void GetWorldLoc(int px, int py, out float wx, out float wy)
      {
         Vector3 v = new Vector3((float)px, (float)py, 0);
         Vector4 res = Vector3.Transform(v, mDevice.Transform.Projection);

         wx = res.X;
         wy = res.Y;
      }
      public float GetWorldLocY(int py)
      {
         float wx;
         float wy;

         GetWorldLoc(0, py, out wx, out wy);
         return wy;
      }
      public float GetWorldLocX(int px)
      {
         float wx;
         float wy;

         GetWorldLoc(px, 0, out wx, out wy);
         return wx;
      }     

      public void DrawBox(int color, RectangleF rect)
      {
         DrawBox(color,rect.Top,rect.Left,rect.Right,rect.Bottom);
      }
      public void DrawBox(int color, float top, float left, float right, float bottom)
      {
         if(mUseBoxBuffer)
         {
            if(mBoxBufferCount > (mBoxBufferSize - 7))
            {
               //out of space
               return;            
            }
            int blendColor = color + 30;

            mBoxBuffer[mBoxBufferCount].X = left;
            mBoxBuffer[mBoxBufferCount].Y = top;
            mBoxBuffer[mBoxBufferCount].Color = blendColor;//color;
            mBoxBufferCount++;
            mBoxBuffer[mBoxBufferCount].X = right;
            mBoxBuffer[mBoxBufferCount].Y = top;
            mBoxBuffer[mBoxBufferCount].Color = blendColor;//color;
            mBoxBufferCount++;
            mBoxBuffer[mBoxBufferCount].X = right;
            mBoxBuffer[mBoxBufferCount].Y = bottom;
            mBoxBuffer[mBoxBufferCount].Color = color;
            mBoxBufferCount++;
            mBoxBuffer[mBoxBufferCount].X = left;
            mBoxBuffer[mBoxBufferCount].Y = top;
            mBoxBuffer[mBoxBufferCount].Color = blendColor;//color;
            mBoxBufferCount++;
            mBoxBuffer[mBoxBufferCount].X = right;
            mBoxBuffer[mBoxBufferCount].Y = bottom;
            mBoxBuffer[mBoxBufferCount].Color = color;
            mBoxBufferCount++;
            mBoxBuffer[mBoxBufferCount].X = left;
            mBoxBuffer[mBoxBufferCount].Y = bottom;
            mBoxBuffer[mBoxBufferCount].Color = color;
            mBoxBufferCount++;

         }
         else
         {
            mBoxVertexArray.Add(new CustomVertex.PositionColored((float) left  , (float) top , 0.5f, color ));
            mBoxVertexArray.Add(new CustomVertex.PositionColored((float) right , (float) top , 0.5f, color));
            mBoxVertexArray.Add(new CustomVertex.PositionColored((float) right , (float) bottom  , 0.5f, color));
            mBoxVertexArray.Add(new CustomVertex.PositionColored((float) left  , (float) top    , 0.5f, color));
            mBoxVertexArray.Add(new CustomVertex.PositionColored((float) right , (float) bottom  , 0.5f, color));
            mBoxVertexArray.Add(new CustomVertex.PositionColored((float) left  , (float) bottom , 0.5f, color));
         }
      }
      public void InitBoxBuffer()
      {
         mBoxBufferCount = 0;
         mBoxBufferSize = 200000;
         mBoxBuffer = new CustomVertex.PositionColored[mBoxBufferSize];
         for(int i = 0; i < mBoxBufferSize; i++)
         {
            mBoxBuffer[i] = new CustomVertex.PositionColored(0,0,0,0);
            mBoxBuffer[i].Z = 0.5f;
         }
         mUseBoxBuffer = true;
      }

      public void FlushBoxes()
      {
         lock(this)
         {
            if(mBoxBufferCount != 0)
            {
               mDevice.VertexFormat = CustomVertex.PositionColored.Format;
               mDevice.DrawUserPrimitives(PrimitiveType.TriangleList, mBoxBufferCount / 3, mBoxBuffer);  
               mBoxBufferCount = 0;
            }

            if(mBoxVertexArray.Count != 0)
            {
               CustomVertex.PositionColored[] verts  = (CustomVertex.PositionColored[])mBoxVertexArray.ToArray(typeof(CustomVertex.PositionColored));
               mDevice.VertexFormat = CustomVertex.PositionColored.Format;
               mDevice.DrawUserPrimitives(PrimitiveType.TriangleList, mBoxVertexArray.Count / 3, verts);  
               mBoxVertexArray.Clear();
            }
         }
      }
      //is there a float or double type...
      public void DrawLine(int color, double x1, double y1, double x2, double y2)
      {
         DrawLine(color, (float)x1, (float)y1, (float)x2, (float)y2);
      }

      public void InitLineBuffer()
      {
         mLineBufferCount = 0;
         mLineBufferSize = 100000;
         mLineBuffer = new CustomVertex.PositionColored[mLineBufferSize];
         for(int i = 0; i < mLineBufferSize; i++)
         {
            mLineBuffer[i] = new CustomVertex.PositionColored(0,0,0,0);
            mLineBuffer[i].Z = 0.5f;
         }
         mUseLineBuffer = true;
      }

      //GC perf is bad when newing each point, but using an array buffer requires you to
      //index a big array many times since references to points vertices dont work...
      //perhaps the dx vertex buffers would be a third option to try...
      //Whatever ends up working best should be applied to the box drawing.
      public void DrawLine(int color, float x1, float y1, float x2, float y2)
      {       
         if(mUseLineBuffer)
         {
            if(mLineBufferCount > (mLineBufferSize - 7))
            {
               //out of space
               return;
            }

            mLineBuffer[mLineBufferCount].X = x1;
            mLineBuffer[mLineBufferCount].Y = y1;
            mLineBuffer[mLineBufferCount].Color = color;
            mLineBufferCount++;

            mLineBuffer[mLineBufferCount].X = x2;
            mLineBuffer[mLineBufferCount].Y = y2;
            mLineBuffer[mLineBufferCount].Color = color;
            mLineBufferCount++;
         }
         else
         {
            mLineVertexArray.Add(new CustomVertex.PositionColored(x1,y1,0.5f,color));
            mLineVertexArray.Add(new CustomVertex.PositionColored(x2,y2,0.5f,color));
         }
         //else if (use vertex buffers???)
      }
      public void FlushLines()
      {
         lock(this)
         {
            if(mLineBufferCount != 0)
            {
               mDevice.VertexFormat = CustomVertex.PositionColored.Format;
               mDevice.DrawUserPrimitives(PrimitiveType.LineList, mLineBufferCount / 2, mLineBuffer);  
               mLineBufferCount = 0;
            }
            if(mLineVertexArray.Count != 0)
            {
               CustomVertex.PositionColored[] verts  = (CustomVertex.PositionColored[])mLineVertexArray.ToArray(typeof(CustomVertex.PositionColored)); 
               mDevice.VertexFormat = CustomVertex.PositionColored.Format;
               mDevice.DrawUserPrimitives(PrimitiveType.LineList, mLineVertexArray.Count / 2, verts);  
               mLineVertexArray.Clear();
            }
         }
      }
      public void FlushAll()
      {
         FlushBoxes();
         FlushLines();
      }
      //if nothing will render and you think you are going insane, you can always call testcrap...
      protected void testCrap()
      {
         int  iTime  = Environment.TickCount % 1000;
         float fAngle = iTime * (2.0f * (float)Math.PI) / 1000.0f;
         mDevice.Transform.World = Matrix.Identity; //Matrix.RotationY( fAngle );
         mDevice.Transform.View = Matrix.LookAtLH( new Vector3( 0.0f, 3.0f,-5.0f ), new Vector3( 0.0f, 0.0f, 0.0f ), new Vector3( 0.0f, 1.0f, 0.0f ) );
         //mDevice.Transform.Projection = Matrix.PerspectiveFovLH( (float)Math.PI / 4, 1.0f, 1.0f, 100.0f );   
         mDevice.Transform.View = Matrix.LookAtLH( new Vector3( 0.0f, 0.0f,-5.0f ), new Vector3( 0.0f, 0.0f, 0.0f ), new Vector3( 0.0f, 1.0f, 0.0f ) );
         mDevice.Transform.Projection = Matrix.OrthoLH(10.0f,5.0f , 1.0f, 100.0f) ;
         CustomVertex.PositionColored[] verts = new CustomVertex.PositionColored[3];
         verts[0].X=-1.0f; verts[0].Y=-1.0f; verts[0].Z=0.0f; verts[0].Color = System.Drawing.Color.DarkGoldenrod.ToArgb();
         verts[1].X=1.0f; verts[1].Y=-1.0f ;verts[1].Z=0.0f; verts[1].Color = System.Drawing.Color.MediumOrchid.ToArgb();
         verts[2].X=0.0f; verts[2].Y=1.0f; verts[2].Z = 0.0f; verts[2].Color = System.Drawing.Color.Cornsilk.ToArgb();
         mDevice.VertexFormat = CustomVertex.PositionColored.Format;
         mDevice.DrawUserPrimitives(PrimitiveType.TriangleList, 1, verts);  
      }
      #endregion

      #region UIHelperCode

      System.DateTime mLastRender = System.DateTime.Now;
      protected double mUIForceRate = 8;//50;//41;//50;//ms
      //Call this to maintain a certain minimum framerate.
      public void UpdateIfNeeded()
      {
         double msWaited = ((System.TimeSpan)(System.DateTime.Now - mLastRender)).TotalMilliseconds;
         if(mUIForceRate < msWaited)
         {
            try
            {
               this.Render();
            }
            catch(Microsoft.DirectX.Direct3D.DriverInternalErrorException ex)
            {
               ErrorHandler.Error(ex.ToString());
            }
         }
      }

      protected float mZoomX = 1;
      protected float mZoomY = 1;
      protected float mZoomInExponent = 5;
      protected float mZoomOutExponent = 12;//was 5;
      protected int mZoomScrollTicks = 100;
 
      public void ZoomY(int ScrollValue, bool update)
      {
         float zoomExponent = (ScrollValue < mZoomScrollTicks/2)?mZoomOutExponent:mZoomInExponent;
         mZoomY = (float)Math.Pow((0.5 + (mZoomScrollTicks - ScrollValue) / (float)mZoomScrollTicks), zoomExponent);
         if(update)
         {
            UpdateIfNeeded();
         }
      }
      public void ZoomX(int ScrollValue, bool update)
      {
         float zoomExponent = (ScrollValue < mZoomScrollTicks/2)?mZoomOutExponent:mZoomInExponent;
         mZoomX = (float)Math.Pow((0.5 + (mZoomScrollTicks - ScrollValue) / (float)mZoomScrollTicks), zoomExponent);
         if(update)
         {
            UpdateIfNeeded();
         }      
      }
      public float GetZoomX(){return mZoomX;}
      public float GetZoomY(){return mZoomY;}

      private ArrayList mLightWeightButtons = new ArrayList();
      public void RegisterButton(UIRegion region)
      {
         mLightWeightButtons.Add(region);
      }
      public void ButtonClickTest(float x, float y)
      {
         foreach(UIRegion region in mLightWeightButtons)
         {
            region.TestClicked(x,y);
         }
      }

//      public void DrawXScale(int color, RectangleF boundingRect, float xOffset, float tickResolution)
//      {
//         int numTics = (int)((rect.Right - rect.Left) / tickResolution);
//         float position;
//         for(int i=1; i<numTics; i++)
//         {
//            position = rect.Left + xOffset + (tickResolution * i);
//            DrawLine(color,position,rect.Top,position,rect.Bottom);
//         }
//      }
//      protected void DrawGrid(RectangleF rect, int color, float xspace, float yspace)
//      {
//         //start from bottom left
//         float xStart = rect.Left;
//         float yStart = rect.Bottom;
//         float xEnd = rect.Right;
//         float yEnd = rect.Top;
//         for(float x = xStart; x < xEnd; x+=xspace)
//         {
//            DrawLine(color,x,yStart,x,yEnd);                                   
//         }
//         for(float y = yStart; y < xEnd; y+=xspace)
//         {
//            DrawLine(color,y,xStart,y,xEnd);                                
//         }
//      }

      public void DrawScale(int color, RectangleF boundingRect, float offset, float tickResolution, bool bIsHorizontal, bool bUnevenRulerTics, bool bRulerTicsTopLeft)
      {
         float measure1,measure2,bound1,bound2;
         if(bIsHorizontal)
         {
            measure1 = boundingRect.Left;
            measure2 = boundingRect.Right;
            bound1 = boundingRect.Top;
            bound2 = boundingRect.Bottom;
         }
         else
         {
            measure1 = boundingRect.Top;
            measure2 = boundingRect.Bottom;
            bound1 = boundingRect.Left;
            bound2 = boundingRect.Right;            
         }
         int numTics = (int)((measure2 - measure1) / tickResolution);
         float position;

         if(offset*offset > tickResolution*tickResolution)
         {
            offset = offset - ((float)Math.Floor(offset/tickResolution)+1) *  tickResolution;
         }
         if(offset*offset > 0)
            numTics++;

         float rulerBound1 = bound1;
         float rulerBound2 = bound2;

         for(int i=1; i<=numTics; i++)
         {
            float tickScale = 1.0f;
            if(bUnevenRulerTics)
            {
               switch (i & 3)
               {
                  case 0: tickScale = 1.0f; break;
                  case 1: tickScale = 0.3f; break;
                  case 2: tickScale = 0.6f; break;
                  case 3: tickScale = 0.3f; break;
               }
            }
            if(bUnevenRulerTics && bRulerTicsTopLeft)
            {
                rulerBound2 = bound2 - ((1-tickScale) * (bound2 - bound1));
               //rulerBound1 =  bound1 + (tickScale * (bound2 - bound1));
            }
            else if(bUnevenRulerTics && !bRulerTicsTopLeft)
            {
               rulerBound1 =  bound1 + ((1-tickScale) * (bound2 - bound1));
               //rulerBound2 = bound2 - (tickScale * (bound2 - bound1));
            }

            position = measure1 + offset + (tickResolution * i);
            if(bIsHorizontal)
            {
               DrawLine(color,position,rulerBound1,position,rulerBound2);
            }
            else
            {
               DrawLine(color,rulerBound1,position,rulerBound2,position);
            }
         }
      }
      public void DrawGrid(int color, RectangleF boundingRect, float xOffset, float xTickResolution, float yOffset, float yTickResolution)
      {
         DrawScale(color,boundingRect,yOffset,yTickResolution,false,false,false);      
         DrawScale(color,boundingRect,xOffset,xTickResolution,true,false,false);  
         //FlushAll();
      }



      static System.Drawing.Color mColor = System.Drawing.Color.Cyan;
      int colorCount = 0;
      protected int getNextColor()
      {
         mColor = System.Drawing.Color.FromArgb(mColor.G,mColor.B,mColor.R);
         colorCount++;
         if((colorCount % 4) == 0)
         {
            mColor = System.Drawing.Color.Red;
         }
         if((colorCount % 7) == 0)
         {
            mColor = System.Drawing.Color.Cyan;
         }
         return mColor.ToArgb();
      }

      const int cColorTableSize = 300;
      private int[] mColorTable = null;
      public int GetColor(int offset)
      {
         if (mColorTable == null)
            InitColorTable(cColorTableSize);
         if(offset < cColorTableSize)
         {
            return mColorTable[offset];
         }
         else
         {
            return getNextColor2(offset);
         }
      }
      protected void InitColorTable(int size)
      {
         mColorTable = new int[size];
         for(int i = 0 ; i < size; i++)
         {
            mColorTable[i] = getNextColor2(i);
         }
      }
      protected int getNextColor2(int count)
      {
         System.Drawing.Color color;
         double r = Math.Abs(Math.Sin(count * 0.3) * 255);
         double g = Math.Abs(Math.Cos(count * 0.7) * 255);
         double b = Math.Abs(Math.Sin(count * 0.9) * 255);
         
         color = System.Drawing.Color.FromArgb((int)r,(int)g,(int)b);
//         if(r+g+b < 255)
//         {
//            return getNextColor2(count++);
//         }  
         return color.ToArgb();
      }

      #endregion

		#region Component Designer generated code
		/// <summary> 
		/// Required method for Designer support - do not modify 
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
         // 
         // DXManagedControl
         // 
         this.Name = "DXManagedControl";

      }
      #endregion

   }

   #region Helper Classes

   public class MouseTrackerVisual
   {
      public bool mbDrawWhenUp;
      public int mUpColor;
      public bool mbDrawWhenDown;
      public int mDownColor;
      public float mWidth;
      public bool mbDrawDownLine;
   
      DXManagedControl mParent;
      public MouseTrackerVisual(DXManagedControl parent)
      {
         mParent = parent;

         mWidth = 6;
         mbDrawWhenUp = true;
         mUpColor = System.Drawing.Color.PeachPuff.ToArgb();
         mbDrawWhenDown = true;
         mDownColor = System.Drawing.Color.LightGoldenrodYellow.ToArgb();
         mbDrawDownLine = true;
      }
      ArrayList mRegions = new ArrayList();
     
      public void AddRegion(UIRegion region)
      {
         mRegions.Add(region);  
      }
      public void ClearRegions()
      {
         mRegions.Clear();
      }

      public UIRegion GetSelectedRegion(float MouseX, float MouseY)
      {
         float fixedMouseY = mParent.Height - MouseY;
         foreach(UIRegion region in mRegions)
         {
            if(region.mRect.Contains(MouseX / mParent.Width, fixedMouseY / mParent.Height))
            {
               return region; 
            }
         }
         return null;
      }

      public void Draw(bool mouseDown, float MouseX, float MouseY)
      {
         if(mRegions.Count == 0)
         {
            Draw(mouseDown, MouseX, MouseY, 0, mParent.Height);      
         }
         else
         {
            UIRegion region = GetSelectedRegion(MouseX, MouseY);
            if(region != null)
            {
               Draw(mouseDown, MouseX, MouseY, region.mRect.Top * mParent.Height, region.mRect.Bottom * mParent.Height);  
            }
         }
      }
      
      //right now this only draws one type of mouse visual..
      public void Draw(bool mouseDown, float MouseX, float MouseY, float top, float bottom)
      {
         if(((mouseDown == true) && (mbDrawWhenDown == true))
            || ((mouseDown == false) && (mbDrawWhenUp == true)))
         {
            int color = (mouseDown)?mDownColor:mUpColor;

            mParent.SetViewToPixels();

            mParent.DrawBox(color,top,MouseX - mWidth/2, MouseX + mWidth/2,bottom);
            
            if(mouseDown && mbDrawDownLine)
            {
               mParent.DrawLine(System.Drawing.Color.Black.ToArgb(),MouseX,top,MouseX,bottom); 
            }
            mParent.FlushAll();
         }          
      }
   }

   public delegate void MouseRegionEvent(UIRegion sender);
   public class UIRegion
   {
      public UIRegion(string name, RectangleF rect)
      {
         mName = name;
         mRect = rect;
      }

      public bool Equals(UIRegion other)
      {
         return mName == other.mName;
      }
      public void TestClicked(float x, float y)
      {
         if(mRect.Contains(x,y) && Clicked != null)
         {
            Clicked(this);
         }
      }
      public event MouseRegionEvent Clicked = null;
      public string mName;
      public RectangleF mRect;

      public bool mbEnabled = true;

      
      public Color mBackGroundColor = Color.Wheat;//hehe

   }

   #endregion
  

}


