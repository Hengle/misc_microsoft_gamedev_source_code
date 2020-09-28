using System;
using System.Collections.Generic;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Diagnostics;

namespace GetBuild
{
   public partial class Form1 : Form
   {
      private ArrayList mBuildFiles = new ArrayList();
      //private Process mProcess = null;
      private ProcessCaller mProcessCaller = null;
      private bool mBatchProcess = false;
      private bool mOptionsLocked = false;

      //============================================================================
      //============================================================================
      public Form1()
      {
         InitializeComponent();
      }

      //============================================================================
      //============================================================================
      private void Form1_Load(object sender, EventArgs e)
      {
         goButton.Enabled = false;
         cancelButton.Enabled = false;
         readOptions();
         getBuildFileList();
         if (buildListView.Items.Count > 0)
            buildListView.Items[0].Selected = true;
      }

      //============================================================================
      //============================================================================
      private void Form1_FormClosing(object sender, FormClosingEventArgs e)
      {
         if (mProcessCaller != null)
            mProcessCaller.Cancel();
         writeOptions();
      }

      //============================================================================
      //============================================================================
      private void resetOptions()
      {
         buildFolderTextBox.Text = @"";
         localFolderTextBox.Text = @"";
         zipFolderTextBox.Text = @"";
         batchFileTextBox.Text = @"";
         deleteFilesCheckBox.Checked = false;
         buildListView.Items.Clear();
         mBuildFiles.Clear();
         lastBuildTextBox.Text = @"";
      }

      //============================================================================
      //============================================================================
      private void readOptions()
      {
         if (mOptionsLocked)
            return;

         mOptionsLocked = true;

         resetOptions();
         
         // Read in options from the GetBuild.txt file
         TextReader reader = null;
         try
         {
            reader = new StreamReader("GetBuild.txt");
         }
         catch 
         { 
            try
            {
               reader = new StreamReader("GetBuildDefaults.txt");
            }
            catch { }
         }
         try
         {
            Regex re = new Regex(@" ");
            if (reader != null)
            {
               try
               {
                  for (; ; )
                  {
                     string line = reader.ReadLine();
                     if (line == null) break;
                     string[] tokens = re.Split(line);
                     bool nextTokenAsBuildFolder = false;
                     bool nextTokenAsLocalFolder = false;
                     bool nextTokenAsZipFolder = false;
                     bool nextTokenAsBatchFile = false;
                     bool nextTokenAsLastBuild = false;
                     foreach (string token in tokens)
                     {
                        if (token.Length == 0)
                           continue;
                        if (token.Substring(0, 1) == ";")
                           break;
                        if (nextTokenAsBuildFolder)
                        {
                           if (buildFolderTextBox.Text == @"")
                              buildFolderTextBox.Text = token;
                           else
                              buildFolderTextBox.Text += " " + token;
                        }
                        else if (nextTokenAsLocalFolder)
                        {
                           if (localFolderTextBox.Text == @"")
                              localFolderTextBox.Text = token;
                           else
                              localFolderTextBox.Text += " " + token;
                        }
                        else if (nextTokenAsZipFolder)
                        {
                           if (zipFolderTextBox.Text == @"")
                              zipFolderTextBox.Text = token;
                           else
                              zipFolderTextBox.Text += " " + token;
                        }
                        else if (nextTokenAsBatchFile)
                        {
                           if (batchFileTextBox.Text == @"")
                              batchFileTextBox.Text = token;
                           else
                              batchFileTextBox.Text += " " + token;
                        }
                        else if (nextTokenAsLastBuild)
                        {
                           if (lastBuildTextBox.Text == @"")
                              lastBuildTextBox.Text = token;
                           else
                              lastBuildTextBox.Text += " " + token;
                        }
                        token.ToLower();
                        if (token.CompareTo(@"buildFolder") == 0)
                        {
                           buildFolderTextBox.Text = @"";
                           nextTokenAsBuildFolder = true;
                        }
                        else if (token.CompareTo(@"localFolder") == 0)
                        {
                           localFolderTextBox.Text = @"";
                           nextTokenAsLocalFolder = true;
                        }
                        else if (token.CompareTo(@"zipFolder") == 0)
                        {
                           zipFolderTextBox.Text = @"";
                           nextTokenAsZipFolder = true;
                        }
                        else if (token.CompareTo(@"batchFile") == 0)
                        {
                           batchFileTextBox.Text = @"";
                           nextTokenAsBatchFile = true;
                        }
                        else if (token.CompareTo(@"lastBuild") == 0)
                        {
                           lastBuildTextBox.Text = @"";
                           nextTokenAsLastBuild = true;
                        }
                        else if (token.CompareTo(@"deleteFiles") == 0)
                           deleteFilesCheckBox.Checked = true;
                     }
                  }
               }
               catch { }
               reader.Close();
            }
         }
         catch { }
         mOptionsLocked = false;
      }

      //============================================================================
      //============================================================================
      private void writeOptions()
      {
         if (mOptionsLocked)
            return;

         if (mProcessCaller != null)
            return;

         try
         {
            string path = @"GetBuild.txt";
            try
            {
               if (File.Exists(path))
               {
                  File.Delete(path);
               }
            }
            catch { }

            TextWriter writer = new StreamWriter(path);

            if (buildFolderTextBox.Text.Length > 0)
               writer.WriteLine("buildFolder " + buildFolderTextBox.Text);
            else
               writer.WriteLine(";buildFolder none");

            if (localFolderTextBox.Text.Length > 0)
               writer.WriteLine("localFolder " + localFolderTextBox.Text);
            else
               writer.WriteLine(";localFolder none");

            if (zipFolderTextBox.Text.Length > 0)
               writer.WriteLine("zipFolder " + zipFolderTextBox.Text);
            else
               writer.WriteLine(";zipFolder none");

            if (batchFileTextBox.Text.Length > 0)
               writer.WriteLine("batchFile " + batchFileTextBox.Text);
            else
               writer.WriteLine(";batchFile none");

            if (lastBuildTextBox.Text.Length > 0)
               writer.WriteLine("lastBuild " + lastBuildTextBox.Text);
            else
               writer.WriteLine(";lastBuild none");

            if (deleteFilesCheckBox.Checked)
               writer.WriteLine("deleteFiles");
            else
               writer.WriteLine(";deleteFiles");

            writer.Close();
         }
         catch { }
      }

      //============================================================================
      //============================================================================
      private void getBuildFileList()
      {
         buildListView.Items.Clear();
         mBuildFiles.Clear();

         // Get the list of build archive files
         string dirPath = buildFolderTextBox.Text;
         if (dirPath == @"")
            dirPath = @".";
         string searchPattern = @"*.7z";
         FileInfo[] fileInfoList = null;
         try
         {
            DirectoryInfo dirInfo = new DirectoryInfo(dirPath);
            fileInfoList = dirInfo.GetFiles(searchPattern);
         }
         catch { }
         if (fileInfoList != null)
         {
            ArrayList files = new ArrayList();
            foreach (FileInfo fileInfo in fileInfoList)
               files.Add(fileInfo);
            files.Sort(new FilesDateComparer());
            foreach (FileInfo fileInfo in files)
            {
               buildListView.Items.Insert(0, fileInfo.Name + "  (" + fileInfo.LastWriteTime.ToString() + ")");
               mBuildFiles.Insert(0, fileInfo.Name);
            }
         }
      }

      //============================================================================
      //============================================================================
      private class FilesDateComparer : IComparer
      {
         public int Compare(object x, object y)
         {

            int iResult;

            FileInfo oFileX = (FileInfo)x;
            FileInfo oFileY = (FileInfo)y;

            if (oFileX.LastWriteTime == oFileY.LastWriteTime)
            {
               iResult = 0;
            }
            else
            {
               if (oFileX.LastWriteTime > oFileY.LastWriteTime)
               {
                  iResult = 1;
               }
               else
               {
                  iResult = -1;
               }
            }

            return iResult;
         }
      }

      //============================================================================
      //============================================================================
      private void pickBuildFolder()
      {
         String saveWorkDir = Directory.GetCurrentDirectory();
         FolderBrowserDialog dialog = new System.Windows.Forms.FolderBrowserDialog();
         dialog.Description = "Select the build folder.";
         dialog.ShowNewFolderButton = false;
         dialog.RootFolder = Environment.SpecialFolder.MyComputer;
         if (buildFolderTextBox.Text.Length == 0)
            dialog.SelectedPath = Directory.GetCurrentDirectory();
         else
            dialog.SelectedPath = buildFolderTextBox.Text;
         DialogResult result = dialog.ShowDialog();
         Directory.SetCurrentDirectory(saveWorkDir);
         if (result == DialogResult.OK)
         {
            buildFolderTextBox.Text = dialog.SelectedPath;
            writeOptions();
            getBuildFileList();
         }
      }

      //============================================================================
      //============================================================================
      private void pickLocalFolder()
      {
         String saveWorkDir = Directory.GetCurrentDirectory();
         FolderBrowserDialog dialog = new System.Windows.Forms.FolderBrowserDialog();
         dialog.Description = "Select the local folder.";
         dialog.ShowNewFolderButton = false;
         dialog.RootFolder = Environment.SpecialFolder.MyComputer;
         if (localFolderTextBox.Text.Length == 0)
            dialog.SelectedPath = Directory.GetCurrentDirectory();
         else
            dialog.SelectedPath = localFolderTextBox.Text;
         DialogResult result = dialog.ShowDialog();
         Directory.SetCurrentDirectory(saveWorkDir);
         if (result == DialogResult.OK)
         {
            localFolderTextBox.Text = dialog.SelectedPath;
            writeOptions();
         }
      }

      //============================================================================
      //============================================================================
      private void pickZipFolder()
      {
         String saveWorkDir = Directory.GetCurrentDirectory();
         FolderBrowserDialog dialog = new System.Windows.Forms.FolderBrowserDialog();
         dialog.Description = "Select the 7-Zip folder.";
         dialog.ShowNewFolderButton = false;
         dialog.RootFolder = Environment.SpecialFolder.MyComputer;
         if (zipFolderTextBox.Text.Length == 0)
            dialog.SelectedPath = Directory.GetCurrentDirectory();
         else
            dialog.SelectedPath = zipFolderTextBox.Text;
         DialogResult result = dialog.ShowDialog();
         Directory.SetCurrentDirectory(saveWorkDir);
         if (result == DialogResult.OK)
         {
            zipFolderTextBox.Text = dialog.SelectedPath;
            writeOptions();
         }
      }

      //============================================================================
      //============================================================================
      private void pickBatchFile()
      {
         string dir = @"";
         if (batchFileTextBox.Text.Length > 0)
            dir = Path.GetDirectoryName(batchFileTextBox.Text);
         else
            dir = Directory.GetCurrentDirectory();
         OpenFileDialog dialog = new OpenFileDialog();
         dialog.InitialDirectory = dir;
         dialog.Filter = "Batch files (*.bat)|*.bat";
         dialog.FilterIndex = 1;
         dialog.RestoreDirectory = true;
         if (dialog.ShowDialog() == DialogResult.OK)
         {
            batchFileTextBox.Text = dialog.FileName;
            writeOptions();
         }
      }

      //============================================================================
      //============================================================================
      private void doGetBuild()
      {
         if (mProcessCaller != null)
            return;

         //logList.Items.Clear();
         if (logList.Items.Count > 0)
            log("");

         if (buildListView.Items.Count == 0 || buildListView.SelectedIndices.Count == 0)
         {
            log("ERROR: No build selected.");
            return;
         }

         // Get the path to 7z.exe and make sure it exists
         string zipPath = zipFolderTextBox.Text;
         if (zipPath.Length > 0)
         {
            string c = zipPath.Substring(zipPath.Length - 1);
            if (c != @"\\")
               zipPath += @"\";
         }
         bool zipExists = false;
         try
         {
            zipExists = File.Exists(zipPath + "7z.exe");
         }
         catch { }
         if (!zipExists)
         {
            log("ERROR: Can't find 7-Zip program.");
            return;
         }

         // Setup path to the build archive
         string buildFile = (String)mBuildFiles[buildListView.SelectedIndices[0]];
         if (buildFile == null)
         {
            log("ERROR: No build file selected.");
            return;
         }
         string buildPath = buildFolderTextBox.Text;
         if (buildPath.Length > 0)
         {
            string c = buildPath.Substring(buildPath.Length - 1);
            if (c != @"\\")
               buildPath += @"\";
         }

         // Setup the local path
         string localPath = localFolderTextBox.Text;
         if (localPath.Length == 0)
         {
            log("ERROR: No local folder specified.");
            return;
         }
         else
         {
            string c = localPath.Substring(localPath.Length - 1);
            if (c != @"\\")
               localPath += @"\";
         }

         goButton.Enabled = false;
         cancelButton.Enabled = true;
         this.Cursor = Cursors.AppStarting;
         log("Getting build " + buildFile + "...");

         lastBuildTextBox.Text = buildFile;

         // Clear out the local folder
         if (deleteFilesCheckBox.Checked)
            deleteLocalFiles(localPath);

         // Run 7z to decompress the build
         mBatchProcess = false;
         mProcessCaller = new ProcessCaller(this);
         mProcessCaller.FileName = zipPath + @"7z.exe";
         mProcessCaller.Arguments = @"x " + buildPath + buildFile + @" -y -o" + localPath;
         mProcessCaller.StdErrReceived += new DataReceivedHandler(writeProcessOutput);
         mProcessCaller.StdOutReceived += new DataReceivedHandler(writeProcessOutput);
         mProcessCaller.Completed += new EventHandler(processCompleted);
         mProcessCaller.Cancelled += new EventHandler(processCanceled);
         //mProcessCaller.Failed += no event handler for this one, yet.
         mProcessCaller.Start();
         //mProcess = new Process();
         //mProcess.StartInfo.FileName = zipPath + @"7z.exe";
         //mProcess.StartInfo.Arguments = @"x " + buildPath + buildFile + @" -y -o" + localPath;
         //mProcess.StartInfo.UseShellExecute = false;
         //mProcess.Start();
      }

      //============================================================================
      //============================================================================
      private void log(string text)
      {
         int index = logList.SelectedIndex;
         int count = logList.Items.Count;
         logList.Items.Add(text);
         if (index == count - 1)
            logList.SelectedIndex = logList.Items.Count - 1;
      }

      //============================================================================
      //============================================================================
      private void writeProcessOutput(object sender, DataReceivedEventArgs e)
      {
         log(e.Text);
      }

      //============================================================================
      //============================================================================
      private void processCompleted(object sender, EventArgs e)
      {
         mProcessCaller = null;

         if (!mBatchProcess && batchFileTextBox.Text != @"")
         {
            log("Executing " + batchFileTextBox.Text + "...");
            mBatchProcess = true;
            mProcessCaller = new ProcessCaller(this);
            mProcessCaller.FileName = batchFileTextBox.Text;
            mProcessCaller.Arguments = @"";
            mProcessCaller.StdErrReceived += new DataReceivedHandler(writeProcessOutput);
            mProcessCaller.StdOutReceived += new DataReceivedHandler(writeProcessOutput);
            mProcessCaller.Completed += new EventHandler(processCompleted);
            mProcessCaller.Cancelled += new EventHandler(processCanceled);
            //mProcessCaller.Failed += no event handler for this one, yet.
            mProcessCaller.Start();
            return;
         }

         goButton.Enabled = true;
         cancelButton.Enabled = false;
         this.Cursor = Cursors.Default;

         writeOptions();

         log("Completed");
      }

      //============================================================================
      //============================================================================
      private void processCanceled(object sender, EventArgs e)
      {
         mProcessCaller = null;
         goButton.Enabled = true;
         cancelButton.Enabled = false;
         this.Cursor = Cursors.Default;

         lastBuildTextBox.Text = @"";
         writeOptions();

         log("Cancelled");
      }

      //============================================================================
      //============================================================================
      private bool deleteLocalFiles(string path)
      {
         bool empty = true;
         string searchPattern = @"*.*";
         FileInfo[] fileInfoList = null;
         DirectoryInfo[] dirInfoList = null;
         try
         {
            DirectoryInfo dirInfo = new DirectoryInfo(path);
            dirInfoList = dirInfo.GetDirectories();
            fileInfoList = dirInfo.GetFiles(searchPattern);
         }
         catch { }
         if (dirInfoList != null)
         {
            foreach (DirectoryInfo dirInfo in dirInfoList)
            {
               if (deleteLocalFiles(dirInfo.FullName))
               {
                  try
                  {
                     Directory.Delete(dirInfo.FullName);
                  }
                  catch { }
               }
            }
         }
         if (fileInfoList != null)
         {
            foreach (FileInfo fileInfo in fileInfoList)
            {
               string name = fileInfo.Name.ToLower();
               if (name == "user.cfg" || name == "xfs.txt")
               {
                  empty = false;
                  continue;
               }
               try
               {
                  File.SetAttributes(fileInfo.FullName, FileAttributes.Normal);
               }
               catch { }
               try
               {
                  fileInfo.Delete();
               }
               catch { }
            }
         }
         return empty;
      }

      //============================================================================
      //============================================================================
      private void buildFolderButton_Click(object sender, EventArgs e)
      {
         pickBuildFolder();
      }

      private void localFolderButton_Click(object sender, EventArgs e)
      {
         pickLocalFolder();
      }

      private void zipFolderButton_Click(object sender, EventArgs e)
      {
         pickZipFolder();
      }


      private void pickBatchFileButton_Click(object sender, EventArgs e)
      {
         pickBatchFile();
      }

      private void goButton_Click(object sender, EventArgs e)
      {
         doGetBuild();
      }

      private void cancelButton_Click(object sender, EventArgs e)
      {
         if (mProcessCaller != null)
            mProcessCaller.Cancel();
      }

      private void buildFolderTextBox_TextChanged(object sender, EventArgs e)
      {
         getBuildFileList();
         writeOptions();
      }

      private void localFolderTextBox_TextChanged(object sender, EventArgs e)
      {
         writeOptions();
      }

      private void zipFolderTextBox_TextChanged(object sender, EventArgs e)
      {
         writeOptions();
      }


      private void batchFileTextBox_TextChanged(object sender, EventArgs e)
      {
         writeOptions();
      }


      private void deleteFilesCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         writeOptions();
      }

      private void buildListView_ItemSelectionChanged(object sender, ListViewItemSelectionChangedEventArgs e)
      {
         if (mProcessCaller == null)
         {
            if (buildListView.SelectedIndices.Count > 0)
               goButton.Enabled = true;
            else
               goButton.Enabled = false;
         }
      }

      private void buildListView_MouseDoubleClick(object sender, MouseEventArgs e)
      {
         if (mProcessCaller == null && buildListView.SelectedIndices.Count > 0)
         {
            doGetBuild();
         }
      }
   }
}