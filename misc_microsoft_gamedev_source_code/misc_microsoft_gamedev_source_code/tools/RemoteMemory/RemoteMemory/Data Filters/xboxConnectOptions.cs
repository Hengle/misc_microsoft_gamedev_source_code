using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace RemoteMemory
{
   public partial class xboxConnectOptions : Form
   {
      public xboxConnectOptions()
      {
         InitializeComponent();
         
         textBox2.Text = mSymbolFileName;
      }


      string mSymbolFileName = GlobalSettings.LastEXEFile;
      
      public string SymbolFileName
      {
         get
         {
            return mSymbolFileName;
         }
      }


      private void button2_Click(object sender, EventArgs e)
      {
         if (!File.Exists(mSymbolFileName))
         {
            MessageBox.Show("Please select a valid exe file for this session");
            return;
         }


         GlobalSettings.LastEXEFile = mSymbolFileName;
         loadOptions.saveOptions();
         GlobalSettings.save();

         this.DialogResult = DialogResult.OK;
         this.Close();
      }

      private void button3_Click(object sender, EventArgs e)
      {
         this.DialogResult = DialogResult.Cancel;
         this.Close();
      }


      private void button4_Click(object sender, EventArgs e)
      {
         OpenFileDialog ofd = new OpenFileDialog();
         ofd.Filter = "Game EXE File (.exe)|*.exe|Game XEX File (.xex)|*.xex";
         if (File.Exists(mSymbolFileName))
            ofd.InitialDirectory = Path.GetDirectoryName(mSymbolFileName);
         else
            ofd.InitialDirectory = Environment.GetFolderPath(System.Environment.SpecialFolder.DesktopDirectory).ToString();

         if (ofd.ShowDialog() == DialogResult.OK)
         {
            mSymbolFileName = ofd.FileName;
            textBox2.Text = mSymbolFileName;
         }
      }

   }
}