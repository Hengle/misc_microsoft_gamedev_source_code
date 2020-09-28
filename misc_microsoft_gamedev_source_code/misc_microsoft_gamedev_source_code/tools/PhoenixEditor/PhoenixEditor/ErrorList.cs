using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace PhoenixEditor
{
   public partial class ErrorList : Xceed.DockingWindows.ToolWindow //: Form
   {
      public ErrorList()
      {
         InitializeComponent();


         myDelegate = new UpdateDel(UpdateInternal);

         label1.Text = "Error Log: " + CoreGlobals.getErrorManager().mErrorLog;
      }



      delegate void UpdateDel(string errorText);
      UpdateDel myDelegate;

      void UpdateInternal(string errorText)
      {
         listView1.Items.Add(new ListViewItem(new string[]{ errorText}));
         //Xceed.Grid.DataRow row = gridControl.DataRows.AddNew();
      }


      public void UpdateList(string errorText)
      {
         this.Invoke(myDelegate, errorText);

      }

      private void Clearbutton_Click(object sender, EventArgs e)
      {
         listView1.Items.Clear();
      }

      private void listView1_DoubleClick(object sender, EventArgs e)
      {
         if(listView1.SelectedItems.Count > 0)
         {
            MessageBox.Show(listView1.SelectedItems[0].Text);

         }



      }

      private void listView1_KeyDown(object sender, KeyEventArgs e)
      {
         e.Handled = true;
      }

      private void listView1_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }


   }



}