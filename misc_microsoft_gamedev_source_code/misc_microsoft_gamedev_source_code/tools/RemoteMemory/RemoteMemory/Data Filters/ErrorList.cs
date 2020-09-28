using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace RemoteMemory
{
   public partial class ErrorList : UserControl
   {
      public ErrorList()
      {
         InitializeComponent();
      }

      private void timer1_Tick(object sender, EventArgs e)
      {
         string str = GlobalErrors.popError();
         while (str != "")
         {
            errorListBox.Items.Add(str);
            str = GlobalErrors.popError();
            if (errorListBox.Items.Count > 100)
            {
               errorListBox.Items.Add("Erros exceeds 100. Stopping logging");
               timer1.Enabled = false;
               return;
            }
         }
         
      }

      private void errorListBox_DoubleClick(object sender, EventArgs e)
      {
         if (errorListBox.SelectedItem == null)
            return;

         MessageBox.Show((string)errorListBox.SelectedItem);
      }

   }
}
