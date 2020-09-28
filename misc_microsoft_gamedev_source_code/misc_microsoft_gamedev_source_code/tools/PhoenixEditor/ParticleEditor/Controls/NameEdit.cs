using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace ParticleSystem
{
   public partial class NameEdit : Form
   {
      public NameEdit()
      {
         InitializeComponent();
      }

      private string mNameText;
      public string NameText
      {
         get { return mNameText; }
         set
         {
            mNameText = value;
            textBox1.Text = mNameText;
         }
      }

      private void textBox1_TextChanged(object sender, EventArgs e)
      {
         mNameText = textBox1.Text;
      }
   }
}