using System;
using System.IO;
using System.Globalization;
using System.Runtime.InteropServices;

namespace EditorCore
{
   #region GDI class
   /// <summary>
   /// Managed class for GDI
   /// </summary>
   public sealed class GDI
   {
      private GDI() { }
      public static long DeleteObject(System.IntPtr value)
      {
         return GDIAPI.DeleteObject(value);
      }

      #region GDI API calls
      /// <summary>
      /// Unmanaged calls to the GDI DLL
      /// </summary>
      internal class GDIAPI
      {
         private GDIAPI() { }
         //Public Declare Function DeleteObject Lib "gdi32" Alias "DeleteObject" (ByVal hObject As Long) As Long
         [DllImport("gdi32.DLL", EntryPoint = "DeleteObject", SetLastError = true,
             CharSet = CharSet.Auto, ExactSpelling = true,
             CallingConvention = CallingConvention.StdCall)]
         public static extern long DeleteObject(System.IntPtr @object);
      }
      #endregion
   }
   #endregion

   public class DevIL : IDisposable
   {
      public DevIL()
      {
         Init();
      }
      private void Init()
      {
         DevILAPI.ilInit();
         DevILAPI.iluInit();
         byte result = DevILAPI.ilutRenderer(DevILAPI.ILUT_WIN32);
         if (result != 1)
            throw new Exception("Unable to initialize 'ilutRenderer' for Win32.");
      }
      /// <summary>
      /// Creates a Win32 bitmap (hBitmap) from the specified file.
      /// </summary>
      /// <param name="file">image location</param>
      /// <returns>Hbitmap</returns>
      public System.IntPtr LoadHBitmap(string file)
      {
         int hDC = 0;
         return DevILAPI.ilutWinLoadImage(System.Runtime.InteropServices.Marshal.StringToCoTaskMemAnsi(file), hDC);
      }
      /// <summary>
      /// Creates an Image object from the specified file.
      /// </summary>
      /// <param name="file"></param>
      /// <returns></returns>
      public System.Drawing.Image LoadImage(string file)
      {
         int hDC = 0;
         System.Drawing.Image img;
         System.IntPtr hBitmap;

         hBitmap = DevILAPI.ilutWinLoadImage(System.Runtime.InteropServices.Marshal.StringToCoTaskMemAnsi(file), hDC);                     
         img = System.Drawing.Image.FromHbitmap(hBitmap);
         GDI.DeleteObject(hBitmap);

         return img;
      }

      public static object sSyncRoot = new object();
      /// <summary>
      /// Releases the resources used by the DevIL DLL.
      /// </summary>
      public void Dispose()
      {
         DevILAPI.ilShutDown();
      }

      #region static functions

      /// <summary>
      /// Creates an Image object from the specified file.
      /// </summary>
      /// <param name="file"></param>
      /// <returns></returns>
      public static System.Drawing.Image LoadImageFromFile(string file)
      {
         System.Drawing.Image img = null;
         DevIL il = new DevIL();
         img = il.LoadImage(file);
         il.Dispose();
         return img;
      }

      #endregion

      #region DevIL API calls
      /// <summary>
      /// Umanaged calls to the DevIL DLL
      /// </summary>
      internal class DevILAPI
      {
         public const int ILUT_WIN32 = 2;

         [DllImport("ilut.DLL", EntryPoint = "ilutWinLoadImage", SetLastError = true,
             CharSet = CharSet.Unicode, ExactSpelling = true,
             CallingConvention = CallingConvention.StdCall)]
         public static extern System.IntPtr ilutWinLoadImage(IntPtr file, long hDC);

         //Public Declare Sub ilInit Lib "devil" ()
         [DllImport("devil.DLL", EntryPoint = "ilInit", SetLastError = true,
             CharSet = CharSet.Auto, ExactSpelling = true,
             CallingConvention = CallingConvention.StdCall)]
         public static extern void ilInit();

         //Public Declare Sub iluInit Lib "ilu" ()
         [DllImport("ilu.DLL", EntryPoint = "iluInit", SetLastError = true,
             CharSet = CharSet.Auto, ExactSpelling = true,
             CallingConvention = CallingConvention.StdCall)]
         public static extern void iluInit();

         //Public Declare Function ilutRenderer Lib "ilut" (ByVal Renderer As Long) As Byte
         [DllImport("ilut.DLL", EntryPoint = "ilutRenderer", SetLastError = true,
             CharSet = CharSet.Auto, ExactSpelling = true,
             CallingConvention = CallingConvention.StdCall)]
         public static extern byte ilutRenderer(int renderer);

         [DllImport("devil.DLL", EntryPoint = "ilShutDown", SetLastError = true,
             CharSet = CharSet.Auto, ExactSpelling = true,
             CallingConvention = CallingConvention.StdCall)]
         public static extern void ilShutDown();
      }
      #endregion
   }
}
