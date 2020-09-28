using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using Terrain;
namespace PhoenixEditor
{
   public partial class TabletSettings : Xceed.DockingWindows.ToolWindow //: Form
   {
      public TabletSettings()
      {
         InitializeComponent();

         Init();
      }

      public void Init()
      {
         foreach(Modifier m in TerrainGlobals.getTerrainFrontEnd().mModifiers.Values)
         {
            ModifierControl c = new ModifierControl();
            c.mModifier = m;
            flowLayoutPanel1.Controls.Add(c);
         }
      }
   }
}