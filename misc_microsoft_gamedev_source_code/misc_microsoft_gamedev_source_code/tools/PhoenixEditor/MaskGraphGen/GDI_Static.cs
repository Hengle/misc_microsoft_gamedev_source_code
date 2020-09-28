using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
//using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace graphapp
{
   public class GDIStatic
   {
      static public SolidBrush SolidBrush_lightGray = new SolidBrush(Color.LightGray);
      static public Pen Pen_DimGray = new Pen(Color.DimGray, 1);
      static public Pen Pen_Black = new Pen(Color.Black, 1);


      static public void dispose()
      {
         SolidBrush_lightGray.Dispose();
      }
   };
}