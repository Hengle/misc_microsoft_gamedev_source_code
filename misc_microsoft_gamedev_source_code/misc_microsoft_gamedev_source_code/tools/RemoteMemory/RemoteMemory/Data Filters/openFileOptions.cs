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
   public partial class openFileOptions : Form
   {
      public openFileOptions()
      {
         InitializeComponent();

         comboBox1.SelectedIndex = (int)mPlaybackSpeed;
         textBox1.Text = mFilename;
         textBox2.Text = mSymbolFileName;
      }

      string mFilename = GlobalSettings.LastBINFile;
      string mSymbolFileName = GlobalSettings.LastEXEFile;
      string mLocalClzz = GlobalSettings.LastEXEFile;
      GlobalSettings.ePlaybackSpeeds mPlaybackSpeed = GlobalSettings.PlaybackSpeed;
      int mThrottleAmount = 1000;

      public string FileName
      {
         get
         {
            return mFilename;
         }
      }

      public string SymbolFileName
      {
         get
         {
            return mSymbolFileName;
         }
      }

      public GlobalSettings.ePlaybackSpeeds PlaybackSpeed
      {
         get
         {
            return mPlaybackSpeed;
         }
      }

      public int ThrottleTime
      {
         get
         {
            if (PlaybackSpeed == GlobalSettings.ePlaybackSpeeds.eSlow)
               return 100000;
            else if (PlaybackSpeed == GlobalSettings.ePlaybackSpeeds.eMedium)
               return 10000;
            else if (PlaybackSpeed == GlobalSettings.ePlaybackSpeeds.eFast)
               return 1000;

            return 0;
         }
      }



      private void button1_Click(object sender, EventArgs e)
      {
         OpenFileDialog ofd = new OpenFileDialog();
         ofd.Filter = "allocLog files (.bin)|*.bin";
         if (File.Exists(mFilename))
            ofd.InitialDirectory = Path.GetDirectoryName(mFilename);
         else
            ofd.InitialDirectory = Environment.GetFolderPath(System.Environment.SpecialFolder.DesktopDirectory).ToString();

         if (ofd.ShowDialog() == DialogResult.OK)
         {
            mFilename = ofd.FileName;
            textBox1.Text = mFilename;
         }
      }

      private void button2_Click(object sender, EventArgs e)
      {
         this.DialogResult = DialogResult.Cancel;
         this.Close();
      }

      private void button3_Click(object sender, EventArgs e)
      {
         if (!File.Exists(mSymbolFileName))
         {
            MessageBox.Show("Please select a valid exe file for this session");
            return;
         }

         if (!File.Exists(mFilename))
         {
            MessageBox.Show("Please select a valid bin file for this session");
            return;
         }

          GlobalSettings.LastBINFile = mFilename;
          GlobalSettings.LastEXEFile = mSymbolFileName;
          GlobalSettings.PlaybackSpeed = mPlaybackSpeed;
          loadOptions.saveOptions();
          GlobalSettings.save();

         this.DialogResult = DialogResult.OK;
         this.Close();
      }

      private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (comboBox1.SelectedIndex == 0)
            mPlaybackSpeed = GlobalSettings.ePlaybackSpeeds.eSlow;
         else if (comboBox1.SelectedIndex == 1)
            mPlaybackSpeed = GlobalSettings.ePlaybackSpeeds.eMedium;
         else if (comboBox1.SelectedIndex == 2)
            mPlaybackSpeed = GlobalSettings.ePlaybackSpeeds.eFast;
         else if (comboBox1.SelectedIndex == 3)
            mPlaybackSpeed = GlobalSettings.ePlaybackSpeeds.eASAP;
      }

      private void button4_Click(object sender, EventArgs e)
      {
         OpenFileDialog ofd = new OpenFileDialog();
         ofd.Filter = "Game EXE File (.exe)|*.exe";
         if (File.Exists(mSymbolFileName))
            ofd.InitialDirectory = Path.GetPathRoot(mSymbolFileName);
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