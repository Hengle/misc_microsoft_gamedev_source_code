using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Drawing;
using System.Drawing.Drawing2D;

namespace ParticleSystem
{
   public partial class AngleControl : UserControl
   {
      public AngleControl()
      {
         InitializeComponent();
      }
     
      private void panel1_Paint(object sender, PaintEventArgs e)
      {
         Pen pen = new Pen(Brushes.Red);         
         e.Graphics.DrawPie(pen, e.ClipRectangle, 0, 45);
      }
   }
}
