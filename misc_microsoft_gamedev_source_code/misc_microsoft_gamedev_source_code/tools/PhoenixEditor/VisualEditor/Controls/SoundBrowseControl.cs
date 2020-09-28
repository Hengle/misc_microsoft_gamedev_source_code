using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using EditorCore;


namespace VisualEditor.Controls
{
   public partial class SoundBrowseControl : UserControl
   {
      private string mFileName = "";

      public delegate void ValueChangedDelegate(object sender, EventArgs e);
      public event ValueChangedDelegate ValueChanged;

      public string FileName
      {
         get { return mFileName; }
         set
         {
            mFileName = value;
            textBox1.Text = mFileName;
         }
      }


      public SoundBrowseControl()
      {
         InitializeComponent();
      }

      private void buttonBrowse_Click(object sender, EventArgs e)
      {
         Dialogs.OpenSoundDialog d = new Dialogs.OpenSoundDialog();
         d.FileName = FileName;

         if (d.ShowDialog() == DialogResult.OK)
         {
            string name = d.FileName;

            FileName = name;


            // call event
            EventArgs args = new EventArgs();
            if (ValueChanged != null)
               ValueChanged(this, args);
         }

      }
   }
}
