using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore
{
   public partial class SimpleFloatProgression : UserControl
   {
      public SimpleFloatProgression()
      {
         InitializeComponent();
      }

      private Xceed.Chart.Core.Chart mChart;
      //private AreaSeries mLine;
      private int mSelectedPoint = 0;

      [Browsable(false)]
      private double mMinX;

      [Browsable(false)]
      private double mMaxX;

      [Browsable(false)]
      private double mMinY;

      [Browsable(false)]
      private double mMaxY;
   }
}
