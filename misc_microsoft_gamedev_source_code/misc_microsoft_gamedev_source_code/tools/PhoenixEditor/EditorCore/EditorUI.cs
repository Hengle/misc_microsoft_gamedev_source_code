
//using System;
//using System.IO;
//using System.Drawing;
//using System.Collections.Generic;
//using System.Text;
//using Microsoft.DirectX;
//using Microsoft.DirectX.Direct3D;
//using Microsoft.DirectX.DirectInput;
//using System.Windows.Forms;


using System;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.DirectX.DirectInput;

namespace EditorCore
{

   #region Terrain Temporaries


   /// <summary>
   /// 
   /// </summary>
   public class UIManager
   {
      static Microsoft.DirectX.DirectInput.Device mInputDevice = null;
      static Control mWindow = null;
      static Control mSubControl = null;
      static TabletSupport mTablet = null;

      static public void InitUIManager(Control window, Control subControl)
      {
         mWindow = window;
         window.MouseWheel += new MouseEventHandler(mSubControl_MouseWheel);

         //mSubControl = subControl;
         //mSubControl.MouseMove += new MouseEventHandler(mWindow_MouseMove);
         //mSubControl.MouseDown += new MouseEventHandler(mSubControl_MouseDown);
         //mSubControl.MouseUp += new MouseEventHandler(mSubControl_MouseUp);
         //mSubControl.MouseWheel += new MouseEventHandler(mSubControl_MouseWheel);

         RedirectSubControlUI(subControl);

         mTablet = new TabletSupport(mSubControl);
         mTablet.Start();

      }

      static Dictionary<Control, object> mSubControls = new Dictionary<Control, object>();
      static public void RedirectSubControlUI(Control subControl)
      {
         mSubControl = subControl;
         if(mSubControls.ContainsKey(subControl) == false)
         {
            mSubControls[subControl] = null;

            mSubControl.MouseMove += new MouseEventHandler(mWindow_MouseMove);
            mSubControl.MouseDown += new MouseEventHandler(mSubControl_MouseDown);
            mSubControl.MouseUp += new MouseEventHandler(mSubControl_MouseUp);
            mSubControl.MouseWheel += new MouseEventHandler(mSubControl_MouseWheel);
            mSubControl.KeyDown += new KeyEventHandler(mSubControl_KeyDown);
            mSubControl.KeyPress += new KeyPressEventHandler(mSubControl_KeyPress);
            mSubControl.MouseMove += new MouseEventHandler(mSubControl_MouseMove);
         }

      }

      static void mSubControl_MouseMove(object sender, MouseEventArgs e)
      {
         if (mSubControl.Focused == false)
            mSubControl.Focus();

        
      }

      static void mSubControl_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      static void mSubControl_KeyDown(object sender, KeyEventArgs e)
      {
         e.Handled = true;
      }

      [DllImport("user32.dll")]
      public static extern IntPtr GetForegroundWindow();
      private static bool mIsActiveApp = true;
      public static bool isActiveApp()
      {
         if (mWindow == null)
            return false;

         //Process current = Process.GetCurrentProcess();
         //Process[] processes = Process.GetProcessesByName(current.ProcessName);
         IntPtr activehWnd = GetForegroundWindow();

         mIsActiveApp = mWindow.Handle == activehWnd;

         return mIsActiveApp;
      }
      static public bool HasFocus()
      {
         if (!mIsActiveApp) return false;

         if (mbPaused) return false;

         Dictionary<Control, object>.Enumerator it = mSubControls.GetEnumerator();
         while (it.MoveNext())
         {
            if (it.Current.Key.Focused == true)
            {
               //if (it.Current.Key.TopLevelControl.Capture == true)
               {

                  return true;
               }
            }
         }
         return false;
      }


      static public void InitKeyBoardCapture()
      {
         FreeDirectInput();
         CooperativeLevelFlags coopFlags;
         coopFlags = CooperativeLevelFlags.Background | CooperativeLevelFlags.NonExclusive;
         mInputDevice = new Microsoft.DirectX.DirectInput.Device(SystemGuid.Keyboard);
         mInputDevice.SetCooperativeLevel(mWindow, coopFlags);
         mInputDevice.Acquire();

      }
      static void FreeDirectInput()
      {
         if (null != mInputDevice)
         {
            mInputDevice.Unacquire();
            mInputDevice.Dispose();
            mInputDevice = null;
         }
      }
      static int X;
      static int Y;
      public static int WheelDelta = 0;
      static bool mbPaused = false;
      static public float currentPressure()
      {
         return mTablet.Pressure;
      }

      static public void Pause()
      {
         mbPaused = true;
      }
      static public void UnPause()
      {
         mbPaused = false;
      }
      static public void SetMousePos(int x, int y)
      {

         X = x;
         Y = y;

         //System.Diagnostics.Debug.WriteLine(string.Format("x {0} y {1}", x, y));
 
         
      }
      public enum eMouseButton
      {
         cLeft,
         cRight,
         cMiddle
      }
      static bool[] mButtonState = new bool[3];
      static public bool GetMouseButtonDown(eMouseButton button)
      {
         if (mbPaused) return false;

         return mButtonState[(int)button];
      }
      static private void mWindow_MouseMove(object sender, MouseEventArgs e)
      {

         UIManager.SetMousePos(e.X, e.Y);
      }
      static int mLastX = 0;
      static int mLastY = 0;
      static public bool Moved()
      {
         if (mbPaused) return false;

         if (mLastX != X || mLastY != Y)
         {
            mLastY = Y;
            mLastX = X;
            return true;
         }
         return false;
      }
      static public bool Moved(int amount)
      {
         if (mbPaused) return false;

         if (amount == 0)
            return Moved();

         if (Math.Pow(X - mLastX, 2) + Math.Pow(Y - mLastY, 2) > amount)
         {
            mLastY = Y;
            mLastX = X;
            return true;
         }
         return false;
      }
      static bool mbClicked = false;
      public static bool Clicked()
      {
         if (mbPaused) return false;

         bool temp = mbClicked;
         mbClicked = false;
         return temp;
      }
      static void mSubControl_MouseWheel(object sender, MouseEventArgs e)
      {
         WheelDelta = e.Delta;
      }
      static int mLastSroke = -1;

      /// <summary>
      /// This is temporary .  Need to add real stroke tracking
      /// </summary>
      /// <returns></returns>

      static public bool NewStroke()
      {
         if (mbPaused) return false;

         if (mLastSroke != mStrokeCount)
         {
            mLastSroke = mStrokeCount;
            return true;
         }
         return false;
      }

      static int mStrokeCount = 0;
      static void mSubControl_MouseUp(object sender, MouseEventArgs e)
      {
         mStrokeCount++;
         if (e.Button == MouseButtons.Left)
         {
            mButtonState[(int)eMouseButton.cLeft] = false;
         }
         if (e.Button == MouseButtons.Right)
         {
            mButtonState[(int)eMouseButton.cRight] = false;
         }
         if (e.Button == MouseButtons.Middle)
         {
            mButtonState[(int)eMouseButton.cMiddle] = false;
         }
      }
      static void mSubControl_MouseDown(object sender, MouseEventArgs e)
      {
         mbClicked = true;
         if (e.Button == MouseButtons.Left)
         {
            mButtonState[(int)eMouseButton.cLeft] = true;
         }
         if (e.Button == MouseButtons.Right)
         {
            mButtonState[(int)eMouseButton.cRight] = true;
         }
         if (e.Button == MouseButtons.Middle)
         {
            mButtonState[(int)eMouseButton.cMiddle] = true;
         }
      }
      static public void GetCursorPos(ref Point pt)
      {
         pt.X = X;
         pt.Y = Y;
      }
      static public void Debug(string text)
      {
         mWindow.Text = text;
      }
      static public int GetAsyncKeyState(int key)
      {
         return 0;
      }
      static public bool GetAsyncKeyStateB(Key key)
      {
         if (mbPaused) return false;

         if (HasFocus() == false)
            return false;

         if (null != mInputDevice)
         {
            KeyboardState state = mInputDevice.GetCurrentKeyboardState();
            return state[key];
         }
         return false;
      }
      //doesn't seem to be needed
      static public void ScreenToClient(ref Point cursorPos)
      {
         System.Drawing.Point p = new System.Drawing.Point(cursorPos.X, cursorPos.Y);
         cursorPos = mWindow.PointToClient(p);
         cursorPos = mSubControl.PointToClient(cursorPos);
      }

      static public System.Drawing.Point ClientSize()
      {
         System.Drawing.Point p = new Point();
         p.X = mSubControl.Width;
         p.Y = mSubControl.Height;

         return p;
      }
   }
   #endregion


}
