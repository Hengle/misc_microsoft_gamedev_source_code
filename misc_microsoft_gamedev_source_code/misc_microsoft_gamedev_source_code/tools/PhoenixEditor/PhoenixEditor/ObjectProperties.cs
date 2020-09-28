using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace PhoenixEditor
{
   public partial class ObjectProperties : Xceed.DockingWindows.ToolWindow //: Form
   {
      public ObjectProperties()
      {
         InitializeComponent();
      }

      private void ObjectPropertyGrid_Click(object sender, EventArgs e)
      {

      }

      public object SelectedObject
      {
         set
         {
            this.ObjectPropertyGrid.SelectedObject = value;
         }
         get
         {
            return this.ObjectPropertyGrid.SelectedObject;
         }

      }
   }
}