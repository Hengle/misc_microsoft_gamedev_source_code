using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using System.Windows.Forms;
using System.Collections.Generic;
using System;
using System.Drawing;
using System.IO;
using EditorCore;

namespace Rendering
{
   public class BRenderDevice
   {
      static private Control mParentWindow;
      static private int mWidth;
      static private int mHeight;
      static private float mZNearPlane = 5.0f;
      static private float mZFarPlane = 10000.0f;

      static private Device mpD3DDevice = null;
      static private bool mbDeviceLost = false;  //warning says this is not used, but it is...
      //static private bool mResizeToActiveWindow = true;

      static public event EventHandler mDeviceLost = null;
      static public event EventHandler mDeviceReset = null;

      static public bool createDevice(Control wind, int width, int height, bool allowDeviceLost)
      {
         return createDevice(wind, width, height, allowDeviceLost, false);
      }

      static public bool createDevice(Control wind,int width, int height,bool allowDeviceLost, bool reloadShaders)
      {
         mParentWindow = wind;
         mWidth = width;
         mHeight = height;

         PresentParameters presentParams = new PresentParameters();
         presentParams.Windowed = true;
         presentParams.SwapEffect = SwapEffect.Discard;
         presentParams.EnableAutoDepthStencil = true;
         presentParams.AutoDepthStencilFormat = DepthFormat.D16;

         presentParams.PresentationInterval = PresentInterval.Immediate;

         try
         {
            mpD3DDevice = new Microsoft.DirectX.Direct3D.Device(0, Microsoft.DirectX.Direct3D.DeviceType.Hardware, mParentWindow, CreateFlags.HardwareVertexProcessing, presentParams);
         }
         catch (System.Exception ex)
         {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("unable to recreate D3D Device");
            Console.ForegroundColor = ConsoleColor.White;
            return false;
         }

         mpD3DDevice.DeviceResizing += new System.ComponentModel.CancelEventHandler(BRenderDevice.dev_DeviceResizing);
       //  if (allowDeviceLost)
         {
            mpD3DDevice.DeviceLost += new EventHandler(dev_DeviceLost);
            mpD3DDevice.DeviceReset += new EventHandler(dev_DeviceReset);
            mpD3DDevice.Disposing += new EventHandler(mpD3DDevice_Disposing);
           
         }

         defaultD3DStates();
         InitVertexTypes();
         if (reloadShaders)
         {

         }
         else
         {
            mTextureManager.init();
            mShaderManager.init();
         }
         mpD3DDevice.Reset(presentParams);
         return (mpD3DDevice != null);
      }

      static public bool ReCreateDevice()
      {
         try
         {
            //mpD3DDevice.Dispose();

            PresentParameters presentParams = new PresentParameters();
            presentParams.Windowed = true;
            presentParams.BackBufferWidth = mWidth;
            presentParams.BackBufferHeight = mHeight;
            presentParams.SwapEffect = SwapEffect.Discard;
            presentParams.EnableAutoDepthStencil = true;
            presentParams.AutoDepthStencilFormat = DepthFormat.D16;

            presentParams.PresentationInterval = PresentInterval.Immediate;

            mpD3DDevice.Reset(presentParams);

            //defaultD3DStates();
            //InitVertexTypes();
            mTextureManager.reloadTexturesIfNeeded(true);//..init();
            mShaderManager.reloadShadersIfNeeded(true);//.init();


            changePerspectiveParams(mFov, mWindowWidth, mWindowHeight);

         }
         catch (System.Exception ex)
         {

            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("unable to recreate D3D");
            Console.ForegroundColor = ConsoleColor.White;
            return false;
         }

         return true;

         //string time = DateTime.Now.ToString();

         //mpD3DDevice.Dispose();
         ////mbDeviceLost = false;
         //bool status =  createDevice(mParentWindow, mWidth, mHeight, true, true);

         //System.Threading.Thread.Sleep(3000);
         //mTextureManager.reloadTexturesIfNeeded(true);
         //mShaderManager.reloadShadersIfNeeded(true);

         //return status;

      }

      static void mpD3DDevice_Disposing(object sender, EventArgs e)
      {
         //throw new Exception("The method or operation is not implemented.");
      }      

      static public bool resizeBackBuffer(int width, int height)
      {
         try
         {
            mWidth = width;
            mHeight = height;

            if (mbDeviceLost == false)
            {
               PresentParameters presentParams = new PresentParameters();
               presentParams.Windowed = true;
               presentParams.BackBufferWidth = mWidth;
               presentParams.BackBufferHeight = mHeight;
               presentParams.SwapEffect = SwapEffect.Discard;
               presentParams.EnableAutoDepthStencil = true;
               presentParams.AutoDepthStencilFormat = DepthFormat.D16;

               presentParams.PresentationInterval = PresentInterval.Immediate;

               mpD3DDevice.Reset(presentParams);
               
            }
            else
            {
               //ReCreateDevice();
               return false;
            }
         }
         catch(System.Exception ex)
         {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("unable to resize D3D backbuffer");
            Console.ForegroundColor = ConsoleColor.White;
            mbDeviceLost = true;
            return false;
         }
         return true;
      }

      static public void destroyDevice()
      {
         mpD3DDevice.Dispose();
         mpD3DDevice = null;

         destroyVertexTypes();
         mTextureManager.destroy();
         mShaderManager.destroy();
      }
      static public Device getDevice()
      {
         return mpD3DDevice;
      }

      static private void dev_DeviceLost(object sender, EventArgs e)
      {

         if (mpD3DDevice!=null)
         {
            lock (mpD3DDevice)
            {
               mbDeviceLost = true;
               if (mDeviceLost != null)
               {
                  mDeviceLost.Invoke(mpD3DDevice, null);
               }
            }
         }
      }
      static private void dev_DeviceResizing(object sender, System.ComponentModel.CancelEventArgs e)
      {
         //  if(mbMinimized == true)
         e.Cancel = true;
      }
      static private void dev_DeviceReset(object sender, EventArgs e)
      {
         mpD3DDevice = (Microsoft.DirectX.Direct3D.Device)sender;

         defaultD3DStates();
         InitVertexTypes();

         //we need to tell everyone we reset here...
         mbDeviceLost = false;

         if (mDeviceReset != null)
         {
            mDeviceReset.Invoke(mpD3DDevice, null);
         }
      }

      static public bool IsDeviceLost()
      {
         return mbDeviceLost;
      }


      static public int getWidth()
      {
         return mWidth;
      }
      static public int getHeight()
      {
         return mHeight;
      }


      static public float getZNearPlane()
      {
         return mZNearPlane;
      }
      static public float getZFarPlane()
      {
         return mZFarPlane;
      }
      static public void setZNearPlane(float near)
      {
         mZNearPlane = near;
      }
      static public void setZFarPlane(float far)
      {
         mZFarPlane = far;
      }


      static public void getParentWindowSize(ref Point p)
      {
         p.X = mParentWindow.Width;
         p.Y = mParentWindow.Height;
      }
      static public void defaultD3DStates()
      {
         mpD3DDevice.SetRenderState(RenderStates.Lighting, false);
         mpD3DDevice.SetRenderState(RenderStates.ZEnable, true);

         
         BRenderDevice.changePerspectiveParams(40, mWidth, mHeight);

         Matrix View = Matrix.Identity;
         mpD3DDevice.SetTransform(TransformType.View, View);

         Matrix World = Matrix.Identity;
         mpD3DDevice.SetTransform(TransformType.World, World);
      }
      static public void changePerspectiveParams(float fov, int width, int height)
      {
         if (mbDeviceLost == true)
            return;

         mpD3DDevice.SetTransform(TransformType.Projection, Matrix.PerspectiveFovLH(Geometry.DegreeToRadian(fov), width / (float)height, mZNearPlane, mZFarPlane));
         mWindowWidth = width;
         mWindowHeight = height;
         mFov = fov;
      }
      static float mFov = 30;
      static int mXOffset = 0;
      static int mYOffset = 0;
      static int mWindowWidth = 0;
      static int mWindowHeight = 0;


      static public void setWindowOffset(int x, int y)
      {
         mXOffset = x;
         mYOffset = y;
      }



      static public void getScreenToD3DCoords(ref Point pt)
      {
         float xrat = (float)(pt.X) / (float)(mParentWindow.Width);
         float yrat = (float)(pt.Y) / (float)(mParentWindow.Height);

         //update the cursor pos to fit into our d3d window
         pt.X = (int)(mWidth * xrat);
         pt.Y = (int)(mHeight * yrat);
      }


      static public Vector3 getRayPosFromMouseCoords(bool farPlane, Point pt)
      {

         Point cursorPos = Point.Empty;

         cursorPos = pt;


         //Point p1 = mParentWindow.PointToScreen(new Point(0, 0));
         //Point p2 = mParentWindow.PointToScreen(new Point(mParentWindow.Width, mParentWindow.Height));

         //int h = p2.Y - p1.Y;

         //// UIManager.ScreenToClient(ref cursorPos);

         //float xrat = (float)(cursorPos.X) / (float)(mParentWindow.Width);// - p1.X);
         //float yrat = (float)(cursorPos.Y) / (float)(mParentWindow.Height);// - p1.Y );

         ////update the cursor pos to fit into our d3d window
         ////cursorPos.X = (int)(mWidth * xrat) + p1.X;
         ////cursorPos.Y = (int)(mHeight * yrat) + p1.Y;

         ////cursorPos.X = (int)(mWidth * xrat);// +p1.X;
         ////cursorPos.Y = (int)(mHeight * yrat);// +p1.Y;

         ////cursorPos.X = (int)(mParentWindow.Width * xrat);// +p1.X;
         ////cursorPos.Y = (int)(mParentWindow.Height * yrat);// +p1.Y;

         //cursorPos.X = (int)((mParentWindow.Width + p1.X) * xrat);// +p1.X;
         //cursorPos.Y = (int)((mParentWindow.Height + p1.Y ) * yrat);// +p1.Y;
         ////mWidth

         //mParentWindow.Width
         float xrat = (float)(cursorPos.X ) / (float)(mWindowWidth);
         float yrat = (float)(cursorPos.Y ) / (float)(mWindowHeight);

         //cursorPos.X = (int)((mWidth + mXOffset) * xrat);
         //cursorPos.Y = (int)((mHeight + mYOffset) * yrat);
         cursorPos.X = (int)((mWidth ) * xrat);
         cursorPos.Y = (int)((mHeight ) * yrat);

         //cursorPos.X = (int)((mWindowWidth + mXOffset) * xrat);
         //cursorPos.Y = (int)((mWindowHeight + mYOffset) * yrat);

         //cursorPos = pt;

        // cursorPos = mParentWindow.PointToScreen(pt);

 
         Vector3 pos = new Vector3(cursorPos.X, cursorPos.Y, (farPlane) ? 1f : 0f);
         Vector3 retval = new Vector3(0, 0, 0);


         Matrix mV, mP, mW;
         mW = BRenderDevice.getDevice().GetTransform(TransformType.World);
         mV = BRenderDevice.getDevice().GetTransform(TransformType.View);
         mP = BRenderDevice.getDevice().GetTransform(TransformType.Projection);
         Viewport vp;
         vp = BRenderDevice.getDevice().Viewport;
         retval = Vector3.Unproject(pos, vp, mP, mV, mW);

         return retval;
      }

      //------------------------------------------------
      //CLM [04.24.06] CALL THIS WHEN CHANGING TABS!!!
      static public void setWindowTarget(Control wind)
      {
         if (mbDeviceLost == true)
            return;

         mParentWindow = wind;

         changePerspectiveParams(40, mParentWindow.Width, mParentWindow.Height);
      }

      //------------------------------------------------
      //abstracted functions to wrap internals
      
      static public void clear(bool zbuf, bool color, int colorVal, float zdepth, int stencil)
      {
         if (mbDeviceLost == true)
            return;

         ClearFlags flags = (zbuf ? ClearFlags.ZBuffer : 0) | (color? ClearFlags.Target:0);

         BRenderDevice.getDevice().Clear(flags, unchecked((int)colorVal), zdepth, stencil);
      }
      static public void beginScene()
      {
         if (mbDeviceLost == true)
            return;
         mpD3DDevice.BeginScene();
      }
      static public void endScene()
      {
         if (mbDeviceLost == true)
            return;
         mpD3DDevice.EndScene();
         mTextureManager.reloadTexturesIfNeeded(false);
         mShaderManager.reloadShadersIfNeeded(false);
      }
      static public void present()
      {
         lock (mpD3DDevice)
         {
            if (mbDeviceLost == true)
               return;
            int result;
            mpD3DDevice.CheckCooperativeLevel(out result);
            if ((int)ResultCode.Success == result)
            {
               mpD3DDevice.Present(mParentWindow);
            }
            else if ((int)ResultCode.DeviceLost == result)
            {
               return;//dont do shit
            }
            else if ((int)ResultCode.DeviceNotReset == result)
            {
               ReCreateDevice();
            }
         }
      }
      //------------------------------------------------
      //------------------------------------------------
      //------------------------------------------------
      //------------------------------------------------
      static public uint D3DCOLOR_COLORVALUE(float r, float g, float b, float a)
      {
         return (uint)((((uint)(a * 255f) & 0xff) << 24) | (((uint)(r * 255f) & 0xff) << 16) | (((uint)(g * 255f) & 0xff) << 8) | ((uint)(b * 255f) & 0xff));
      }

      static public void InitVertexTypes()
      {
         VertexTypes.Pos p = new VertexTypes.Pos(0,0,0);
         VertexTypes.Pos_Color pc = new VertexTypes.Pos_Color(0, 0, 0, 0);
         VertexTypes.Pos_Color_uv0 pcu = new VertexTypes.Pos_Color_uv0(0, 0, 0, 0,0,0);
         VertexTypes.Pos_Normal_uv0 pnu = new VertexTypes.Pos_Normal_uv0(0, 0, 0, 0, 0,0,0,0 );
         VertexTypes.Pos_uv0 pu = new VertexTypes.Pos_uv0(0, 0, 0, 0, 0);
         VertexTypes.Pos_uv0_uv1 pu01 = new VertexTypes.Pos_uv0_uv1(0, 0, 0, 0, 0, 0, 0);
         VertexTypes.PosW_uv0 pw_uv0 = new VertexTypes.PosW_uv0(0, 0, 0, 0, 0, 0);
         VertexTypes.PosW_uv0_uv1 pw_uv0_uv1 = new VertexTypes.PosW_uv0_uv1(0, 0, 0, 0, 0, 0, 0, 0);
         VertexTypes.Pos16 p16 = new VertexTypes.Pos16(0, 0, 0);
         VertexTypes.Pos16_Color p16c = new VertexTypes.Pos16_Color(0, 0, 0, 0);
         
      }
      static private void destroyVertexTypes()
      {
         if(VertexTypes.Pos.vertDecl!=null)
         {
            VertexTypes.Pos.vertDecl.Dispose();
            VertexTypes.Pos.vertDecl = null;
         }
         if (VertexTypes.Pos_Color.vertDecl != null)
         {
            VertexTypes.Pos_Color.vertDecl.Dispose();
            VertexTypes.Pos_Color.vertDecl = null;
         }
         if (VertexTypes.Pos_Color_uv0.vertDecl != null)
         {
            VertexTypes.Pos_Color_uv0.vertDecl.Dispose();
            VertexTypes.Pos_Color_uv0.vertDecl = null;
         }
         if (VertexTypes.Pos_Normal_uv0.vertDecl != null)
         {
            VertexTypes.Pos_Normal_uv0.vertDecl.Dispose();
            VertexTypes.Pos_Normal_uv0.vertDecl = null;
         }
         if (VertexTypes.Pos_uv0.vertDecl != null)
         {
            VertexTypes.Pos_uv0.vertDecl.Dispose();
            VertexTypes.Pos_uv0.vertDecl = null;
         }
         if (VertexTypes.PosW_uv0.vertDecl != null)
         {
            VertexTypes.PosW_uv0.vertDecl.Dispose();
            VertexTypes.PosW_uv0.vertDecl = null;
         }
         if (VertexTypes.Pos16.vertDecl != null)
         {
            VertexTypes.Pos16.vertDecl.Dispose();
            VertexTypes.Pos16.vertDecl = null;
         }
         if (VertexTypes.Pos16_Color.vertDecl != null)
         {
            VertexTypes.Pos16_Color.vertDecl.Dispose();
            VertexTypes.Pos16_Color.vertDecl = null;
         }
      }
      static public void writeTextureToFile(Texture d3dtex, String filename)
      {
         Image img = Image.FromStream( TextureLoader.SaveToStream(ImageFileFormat.Bmp, (BaseTexture)d3dtex));
         img.Save(filename);
      }

      //-----------------------------
      static private TextureManager mTextureManager = new TextureManager();
      static public TextureManager getTextureManager() { return mTextureManager;}
      static private ShaderManager mShaderManager = new ShaderManager();
      static public ShaderManager getShaderManager() { return mShaderManager; }
   }

   public class VertexTypes
   {
      //CLM [03.20.06] ENSURE THAT YOU'RE ADDING ANY NEW VERTEX TYPES TO BRenderDevice.InitVertexTypes()
      public struct Pos
      {
         public float x, y, z;


         public Pos(float _x, float _y, float _z)
         {
            x = _x; y = _y; z = _z;

            if (Pos.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               Pos.vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);

            }

         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position;
         public static VertexDeclaration vertDecl = null;
      };

      public struct Pos_uv0
      {
         public float x, y, z;
         public float u0, v0;


         public Pos_uv0(float _x, float _y, float _z, float _u0, float _v0)
         {
            x = _x; y = _y; z = _z;
            u0 = _u0;
            v0 = _v0;

            if (Pos_uv0.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			   {
				   new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				   new VertexElement(0, 12, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
				   VertexElement.VertexDeclarationEnd,
			   };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }


         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Texture1;
         public static VertexDeclaration vertDecl = null;
      };
      public struct Pos_uv0_uv1
      {
         public float x, y, z;
         public float u0, v0;
         public float u1, v1;


         public Pos_uv0_uv1(float _x, float _y, float _z, float _u0, float _v0, float _u1, float _v1)
         {
            x = _x; y = _y; z = _z;
            u0 = _u0;
            v0 = _v0;
            u1 = _u1;
            v1 = _v1;

            if (Pos_uv0_uv1.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			   {
				   new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				   new VertexElement(0, 12, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
               new VertexElement(0, 20, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 1),
				   VertexElement.VertexDeclarationEnd,
			   };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }


         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Texture2;
         public static VertexDeclaration vertDecl = null;
      };

      public struct Pos_Color
      {
         public float x, y, z;
         public int color;


         public Pos_Color(float _x, float _y, float _z, int _color)
         {
            x = _x; y = _y; z = _z;
            color = _color;

            if (Pos_Color.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			   {
				   new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				   new VertexElement(0, 12, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.Color, 0),
				   
				   VertexElement.VertexDeclarationEnd,
			   };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }

         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Diffuse;
         public static VertexDeclaration vertDecl = null;
      };

      public struct Pos_Color_uv0
      {
         public float x, y, z;
         public int color;
         public float u, v;


         public Pos_Color_uv0(float _x, float _y, float _z, float _u, float _v, int _color)
         {
            x = _x; y = _y; z = _z;
            u = _u; v = _v;
            color = _color;

            if (Pos_Color_uv0.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			   {
				   new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				   new VertexElement(0, 12, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.Color, 0),
               new VertexElement(0, 24, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
				   VertexElement.VertexDeclarationEnd,
			   };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }

         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Diffuse | VertexFormats.Texture1;
         public static VertexDeclaration vertDecl = null;
      };

      public struct Pos_Normal_uv0
      {
         public float x, y, z;
         public float nx, ny, nz;
         public float u, v;


         public Pos_Normal_uv0(float _x, float _y, float _z, float _nx, float _ny, float _nz, float _u, float _v)
         {
            x = _x; y = _y; z = _z;
            nx = _nx; ny = _ny; nz = _nz;
            u = _u; v = _v;

            if (Pos_Normal_uv0.vertDecl == null)
            {

               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      new VertexElement(0, 12, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Normal, 0),
                  new VertexElement(0, 24, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
				      new VertexElement(0, 32, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Tangent, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Normal | VertexFormats.Texture1;
         public static VertexDeclaration vertDecl = null;
      };

      public struct PosW_uv0
      {
         public float x, y, z, w;
         public float u, v;
         public PosW_uv0(float _x, float _y, float _z, float _w, float _u0, float _v0)
         {
            x = _x;
            y = _y;
            z = _z;
            w = _w;
            u = _u0;
            v = _v0;
            if (PosW_uv0.vertDecl == null)
            {

               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      new VertexElement(0, 16, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
         }
         public static readonly VertexFormats FVF_Flags = VertexFormats.Transformed | VertexFormats.Texture1;
         public static VertexDeclaration vertDecl = null;
      }
      public struct PosW_uv0_uv1
      {
         public float x, y, z, w;
         public float u0, v0;
         public float u1, v1;
         public PosW_uv0_uv1(float _x, float _y, float _z, float _w, float _u0, float _v0, float _u1, float _v1)
         {
            x = _x;
            y = _y;
            z = _z;
            w = _w;
            u0 = _u0;
            v0 = _v0;
            u1 = _u1;
            v1 = _v1;
            if (PosW_uv0_uv1.vertDecl == null)
            {

               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      new VertexElement(0, 16, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
                  new VertexElement(0, 24, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 1),
				      VertexElement.VertexDeclarationEnd,
			      };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
         }
         public static readonly VertexFormats FVF_Flags = VertexFormats.Transformed | VertexFormats.Texture2;
         public static VertexDeclaration vertDecl = null;
      }
      public struct PosW_uv03
      {
         public float x, y, z, w;
         public float u, v, t;
         public PosW_uv03(float _x, float _y, float _z, float _w, float _u0, float _v0, float _t0)
         {
            x = _x;
            y = _y;
            z = _z;
            w = _w;
            u = _u0;
            v = _v0;
            t = _t0;
            if (PosW_uv03.vertDecl == null)
            {

               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      new VertexElement(0, 16, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
         }
         public static readonly VertexFormats FVF_Flags = VertexFormats.Transformed | VertexFormats.Texture1;
         public static VertexDeclaration vertDecl = null;
      }
      public struct PosW_color
      {
         public float x, y, z, w;
         public int color;

         public PosW_color(float _x, float _y, float _z, float _w, int _color)
         {
            x = _x;
            y = _y;
            z = _z;
            w = _w;
            color = _color;
            if (PosW_color.vertDecl == null)
            {

               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.PositionTransformed, 0),
				      new VertexElement(0, 16, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.Color, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Transformed | VertexFormats.Diffuse;
         public static VertexDeclaration vertDecl = null;




      };


      public struct Pos16
      {
         public float16 x, y, z, w;


         public Pos16(float _x, float _y, float _z)
         {
            x = new float16(_x); y = new float16(_y); z = new float16(_z);
            w = new float16(0);

            if (Pos16.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float16Four, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               Pos16.vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);

            }

         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position;
         public static VertexDeclaration vertDecl = null;
      };
      public struct Pos16_Color
      {
         public ushort x, y, z, w;
         public int color;


         public Pos16_Color(float _x, float _y, float _z, int _color)
         {
            float16 tmp = new float16(0);
            tmp.fromFloat(_x); x = tmp.getInternalDat();
            tmp.fromFloat(_y); y = tmp.getInternalDat();
            tmp.fromFloat(_z); z = tmp.getInternalDat();
            w = 0;

            color = _color;

            if (Pos16_Color.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float16Four, DeclarationMethod.Default, DeclarationUsage.Position, 0),
                  new VertexElement(0, 8, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.Color, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               Pos16_Color.vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);

            }

         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Diffuse;
         public static VertexDeclaration vertDecl = null;
      };

      public enum eVertexDeclElement
      {
         cVDE_Position = 0,
         cVDE_BlendWeight1,
         cVDE_BlendWeight4,

         cVDE_BlendIndicies,

         cVDE_Normal,

         cVDE_ColorDWORD,

         cVDE_TexCoord1,
         cVDE_TexCoord2,
         cVDE_TexCoord3,


         cVDE_Tangent,
         cVDE_BiNormal,

         cVDE_Pos16_4,
      };
      public static VertexDeclaration genVertexDecl(List<eVertexDeclElement> decls, bool sortForFVF, ref short vertexStructSize)
      {
         if (sortForFVF)
            decls.Sort();
         List<VertexElement> vList = new List<VertexElement>();
         vertexStructSize = 0;
         int textureCount = 0;
         for (int i = 0; i < decls.Count; i++)
         {
            switch (decls[i])
            {
               case eVertexDeclElement.cVDE_Position:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0));
                  vertexStructSize += sizeof(float) * 3;
                  break;

               case eVertexDeclElement.cVDE_BlendIndicies:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.BlendIndices, 0));
                  vertexStructSize += sizeof(int);
                  break;

               case eVertexDeclElement.cVDE_BlendWeight1:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float1, DeclarationMethod.Default, DeclarationUsage.BlendWeight, 0));
                  vertexStructSize += sizeof(float);
                  break;

               case eVertexDeclElement.cVDE_BlendWeight4:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.BlendWeight, 0));
                  vertexStructSize += sizeof(int);
                  break;

               case eVertexDeclElement.cVDE_Normal:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Normal, 0));
                  vertexStructSize += sizeof(float) * 3;
                  break;
               case eVertexDeclElement.cVDE_Tangent:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Tangent, 0));
                  vertexStructSize += sizeof(float) * 3;
                  break;
               case eVertexDeclElement.cVDE_BiNormal:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.BiNormal, 0));
                  vertexStructSize += sizeof(float) * 3;
                  break;

               case eVertexDeclElement.cVDE_TexCoord2:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, (byte)textureCount));
                  vertexStructSize += sizeof(float) * 2;
                  textureCount++;
                  break;

               case eVertexDeclElement.cVDE_ColorDWORD:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.Color, 0));
                  vertexStructSize += sizeof(int);
                  break;

               case eVertexDeclElement.cVDE_Pos16_4:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float16Four, DeclarationMethod.Default, DeclarationUsage.Position, 0));
                  vertexStructSize += sizeof(short) * 4;
                  break;

            }

         }

         VertexElement[] elements = new VertexElement[vList.Count + 1];
         int c = 0;
         for (c = 0; c < vList.Count; c++)
            elements[c] = vList[c];
         

         //CLM HACKOTRON
         //add in elements for instancing
         //elements[vList.Count + 0] = new VertexElement(1, 0, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 4);
         //elements[vList.Count + 1] = new VertexElement(1, 16, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 5);
         //elements[vList.Count + 2] = new VertexElement(1, 32, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 6);
         //elements[vList.Count + 3] = new VertexElement(1, 48, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 7);

         elements[vList.Count] = VertexElement.VertexDeclarationEnd;

         return new VertexDeclaration(BRenderDevice.getDevice(), elements);


      }
   };
}