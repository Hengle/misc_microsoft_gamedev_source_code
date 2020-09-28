using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace RemoteMemory
{
   public partial class LoadOptions : UserControl
   {
      public LoadOptions()
      {
         InitializeComponent();

         numericUpDown1.Value = GlobalSettings.MaxStackWalkDepth;
      }

      public void saveOptions()
      {
         GlobalSettings.MaxStackWalkDepth = (uint)numericUpDown1.Value;
      }

      private void numericUpDown1_ValueChanged(object sender, EventArgs e)
      {
      }
   }
}
