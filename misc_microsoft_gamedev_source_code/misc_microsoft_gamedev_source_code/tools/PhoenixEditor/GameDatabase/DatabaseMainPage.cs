using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace GameDatabase
{
   public partial class DatabaseMainPage : EditorCore.BaseClientPage
   {
      public DatabaseMainPage()
      {
         InitializeComponent();



         mDynamicMenus.BuildDynamicMenus(toolStripContainer1, menuStrip1);

      }
   }
}
