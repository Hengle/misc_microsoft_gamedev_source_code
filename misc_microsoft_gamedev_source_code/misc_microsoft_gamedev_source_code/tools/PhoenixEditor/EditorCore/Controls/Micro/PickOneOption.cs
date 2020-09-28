using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace EditorCore
{
   public partial class PickOneOption : Form
   {
      public PickOneOption()
      {
         InitializeComponent();
         this.OKButton.Enabled = false;
      }

      public ICollection<string> Options
      {
         set
         {
            listBox1.Items.Clear();
            if(value != null)
            {
            foreach (string s in value)
            {
               this.listBox1.Items.Add(s);
            }
            }
         }

      }
      string mSelectedOption = "";
      public string SelectedOption
      {
         get
         {
            return this.listBox1.SelectedItem.ToString();
         }
      }

      private void OKButton_Click(object sender, EventArgs e)
      {
         this.DialogResult = DialogResult.OK;
         this.Close();
      }

      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.DialogResult = DialogResult.Cancel;
         this.Close();
      }

      private void listBox1_Click(object sender, EventArgs e)
      {
         if(this.listBox1.SelectedItems.Count > 0)
            this.OKButton.Enabled = true;
      }
   }
}