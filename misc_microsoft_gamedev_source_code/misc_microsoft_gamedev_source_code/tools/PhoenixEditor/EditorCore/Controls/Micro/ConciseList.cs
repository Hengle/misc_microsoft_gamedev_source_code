using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls.Micro
{
   public partial class ConciseList : UserControl
   {
      public ConciseList()
      {
         InitializeComponent();
      }

      public void SetItems(ICollection<string> items)
      {
         listBox1.Items.Clear();
         listBox1.Sorted = true;
        
         List<string> temp = new List<string>();
         temp.AddRange(items);
         listBox1.Items.AddRange(temp.ToArray());
      }



      private void EditButton_Click(object sender, EventArgs e)
      {


      }
   }
}
