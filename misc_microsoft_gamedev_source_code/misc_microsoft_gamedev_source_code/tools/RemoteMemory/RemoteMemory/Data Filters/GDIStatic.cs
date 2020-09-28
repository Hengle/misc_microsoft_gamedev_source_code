using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.Drawing.Imaging;
namespace RemoteMemory
{
   class GDIStatic
   {
      static public SolidBrush SolidBrush_LightGray = new SolidBrush(Color.LightGray);
      static public SolidBrush SolidBrush_DimGray = new SolidBrush(Color.DimGray);
      static public SolidBrush SolidBrush_DarkGray = new SolidBrush(Color.DarkGray);
      static public SolidBrush SolidBrush_Gray = new SolidBrush(Color.Gray);
      static public SolidBrush SolidBrush_Black = new SolidBrush(Color.Black);
      static public SolidBrush SolidBrush_White = new SolidBrush(Color.White);

      static public Pen Pen_LightGray = new Pen(Color.LightGray, 1);
      static public Pen Pen_DimGray = new Pen(Color.DimGray, 1);
      static public Pen Pen_DarkGray = new Pen(Color.DarkGray, 1);
      static public Pen Pen_Gray = new Pen(Color.Gray, 1);
      static public Pen Pen_Black = new Pen(Color.Black, 1);

      static public Font Font_Console_10 = new Font("console", 10);

      static public void dispose()
      {
         SolidBrush_LightGray.Dispose();
         SolidBrush_DimGray.Dispose();
         SolidBrush_DarkGray.Dispose();
      }


      static Color[] DesiredColors = 
      {
         Color.FromArgb(255,0,0),
         Color.FromArgb(0,255,0),
         Color.FromArgb(0,0,255),
         Color.FromArgb(255,0,255),
         Color.FromArgb(255,255,0),
         Color.FromArgb(0,255,255),

         Color.FromArgb(192,64,0),
         Color.FromArgb(0,192,255),
         Color.FromArgb(128,0,192),
         Color.FromArgb(192,0,64),
         Color.FromArgb(192,192,0),
         Color.FromArgb(0,192,192),

         Color.FromArgb(128,0,255),
         Color.FromArgb(0,128,255),
         Color.FromArgb(255,0,128),
         Color.FromArgb(128,0,255),
         Color.FromArgb(255,128,0),
         Color.FromArgb(255,128,128),
      };

      static int mCurrColorIndex = 0;
      static public Color getNextDesiredColor()
      {
         if(mCurrColorIndex >= DesiredColors.Length)
            mCurrColorIndex = 0;

         return DesiredColors[mCurrColorIndex++];
      }


      static public Color CommonBGColor = Color.FromArgb(255, 63, 63, 63);
      static public SolidBrush SolidBrush_CommonBGColor = new SolidBrush(CommonBGColor);

   }
}
