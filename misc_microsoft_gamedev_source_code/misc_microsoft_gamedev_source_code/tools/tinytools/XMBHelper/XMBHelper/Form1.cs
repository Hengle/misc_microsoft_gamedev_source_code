using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Threading;


namespace XMBHelper
{
   public partial class Form1 : Form
   {
      FileSystemWatcher mWatcher = new FileSystemWatcher();
      string mToolPath;
      public Form1()
      {
         InitializeComponent();

         mWatcher.Changed += new FileSystemEventHandler(mWatcher_Changed);
         mWatcher.Created += new FileSystemEventHandler(mWatcher_Created);
         mWatcher.Deleted += new FileSystemEventHandler(mWatcher_Deleted);
         //mWatcher.NotifyFilter = NotifyFilters.LastAccess | NotifyFilters.LastWrite | NotifyFilters.Size;
         mWatcher.Path = computeGameDir() + @"\data\";
         mWatcher.Filter = "*.*";
         mWatcher.IncludeSubdirectories = true;
         mToolPath = computeGameDir() + @"\tools\xmlComp\xmlComp.exe";

         mWatcher.EnableRaisingEvents = true;

         Thread t = new Thread(new ThreadStart(CheckForUpdates));
         t.Start();

      }


      void mWatcher_Deleted(object sender, FileSystemEventArgs e)
      {
         //throw new Exception("The method or operation is not implemented.");

      }

      void mWatcher_Created(object sender, FileSystemEventArgs e)
      {
         //throw new Exception("The method or operation is not implemented.");
      }

      void mWatcher_Changed(object sender, FileSystemEventArgs e)
      {
         string ext = Path.GetExtension(e.FullPath).ToLower();
         if (this.PauseCheckBox.Checked == false)
         {
            if (ext == ".tactics" || ext == ".xml")
            {
               XMBProcessor.CreateXMB(mToolPath, e.FullPath, this.CheckOutXMBBox.Checked);
               this.Invoke((MethodInvoker)(delegate() { Text = System.DateTime.Now.ToShortTimeString(); }));
            }
            if(ext == ".tmp")
            {
               lock(this)
               {
                  mFilesToCheck.Add(e.FullPath);
               }
            }
         }
      }
      List<string> mFilesToCheck = new List<string>();
      bool mbRunning = true;

      void CheckForUpdates()
      {
         while(mbRunning)
         {
            Thread.Sleep(1000);

            lock(this)
            {
               if (mFilesToCheck.Count > 0)
               {
                  Thread.Sleep(1000);
               }
               foreach(string fileName in mFilesToCheck)
               {
                  int index = fileName.IndexOf("~");
                  if (index == -1)
                     continue;
                  string realName = fileName.Substring(0, index);
                  if (((TimeSpan)(DateTime.Now - File.GetLastAccessTime(realName))).TotalMinutes < 2)
                  {
                     XMBProcessor.CreateXMB(mToolPath, realName, this.CheckOutXMBBox.Checked);
                  }
               }
               mFilesToCheck.Clear();
            }
         }

      }

      //the rest is copied from phoenix editor///////////////////////////////////////////////////////


      string computeGameDir() 
      {
         string gameDir = AppDomain.CurrentDomain.BaseDirectory;// mBaseDirectory;
         try
         {

            if (gameDir.Contains("\\work\\"))
            {
               gameDir = gameDir.Substring(0, gameDir.IndexOf("\\work\\")) + "\\work";
            }
            else if (gameDir.Contains("\\production\\"))
            {
               gameDir = gameDir.Substring(0, gameDir.IndexOf("\\production\\")) + "\\production\\work";
            }
            else if (gameDir.Contains("\\xbox\\"))
            {
               gameDir = gameDir.Substring(0, gameDir.IndexOf("\\xbox\\")) + "\\xbox\\work";
            }
            else if (gameDir.Contains("\\x\\"))
            {
               gameDir = gameDir.Substring(0, gameDir.IndexOf("\\x\\")) + "\\x\\work";
            }
            else if (gameDir.Contains("\\X\\"))
            {
               gameDir = gameDir.Substring(0, gameDir.IndexOf("\\X\\")) + "\\X\\work";
            }
            else
            {
               return "";
            }
         }
         catch (System.Exception ex)
         {
            MessageBox.Show(ex.ToString());
         }
         return gameDir;
      }

   }

   public class XMBProcessor
   {

      static public void CreateXMB(string toolpath, string filename, bool UsePerforce)
      {
         try
         {
            //xmlComp –outsamedir –checkout –errormessagebox –pauseonwarnings –file filename.xml
            //Where “filename.xml” can be any XML file (with any extension).

            if (File.Exists(toolpath) == false)
            {
               MessageBox.Show("Can't find: " + toolpath, "Error exporting " + filename);
               return;
            }

            string arguments = "";
            arguments = arguments + " -outsamedir";
            arguments = arguments + " -errormessagebox";
            arguments = arguments + " -file " + filename;
            if (UsePerforce == true)
            {
               arguments = arguments + " -checkout";
            }

            System.Diagnostics.Process xmbUtility;
            
            xmbUtility = new System.Diagnostics.Process();
            System.Diagnostics.ProcessStartInfo se = new System.Diagnostics.ProcessStartInfo(toolpath, arguments);
            se.CreateNoWindow = true;
            se.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;
            xmbUtility = System.Diagnostics.Process.Start(se);
            xmbUtility.WaitForExit();
            xmbUtility.Close();

         }
         catch (System.Exception ex)
         {
            MessageBox.Show(ex.ToString());
         }
      }
   }

}