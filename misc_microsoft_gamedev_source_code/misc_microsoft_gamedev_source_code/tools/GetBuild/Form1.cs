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
using System.Threading;
using XDevkit;
using System.Xml;
using System.Xml.Serialization;
using System.Net;
using System.Net.Sockets;

namespace GetBuild
{
   public partial class Form1 : Form
   {
      private const int cUpdateInterval = 250;
      enum ProcessType { cNone, cGet, cBatch, cDeleteBeforeInternalCopy, cDeleteBeforeXbcpCopy, cInternalCopy, cXbcpCopy, cXfsCopy, cDelete, cReboot, cEmulate };
      private ArrayList mBuildFiles = new ArrayList();
      private ProcessCaller mProcessCaller = null;
      private ProcessType mProcessType = ProcessType.cNone;
      private bool mProcessAutoCopyAndLaunch = false;
      private bool mProcessAutoEmulateAndLaunch = false;
      private bool mOptionsLocked = false;
      private Thread mCopyThread = null;
      public static string mWorkPath = null;
      static string mCopyLocalFolder = null;
      static string mCopyXboxFolder = null;
      static bool mCopySeparateFolder = false;
      static string mCopyXboxName = null;
      static bool mCopyCancel = false;
      static bool mCopyFailed = false;
      static bool mCopyComplete = false;
      bool mCopyAutoLaunch = false;
      static public ArrayList mEvents1 = null;
      static public ArrayList mEvents2 = null;
      static public ArrayList mCurrentEvents = null;
      private System.Windows.Forms.Timer mUpdateTimer;
      private Settings mSettings = null;
      private FileSets mFileSets = null;
      private bool mCopying = false;
      private bool mCopyServerConnected = false;
      private Int32 mCopyTime = 0;
      private bool mAutoLaunchDelay = false;

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
         try
         {
            string hostName = Dns.GetHostName();
            IPAddress[] ips = Dns.GetHostAddresses(hostName);
            foreach (IPAddress ip in ips)
            {
               ipDropdown.Items.Add(ip.ToString());
               if (ipDropdown.Text == "")
                  ipDropdown.Text = ip.ToString();
            }
         }
         catch { }


         mEvents1 = new ArrayList();
         mEvents2 = new ArrayList();
         mCurrentEvents = mEvents1;

         setupWorkPath();
         loadFileSets();
         disableButtons();
         getFileSets();
         readOptions();
         getBuildFileList();
 
         // Set default build to the first one in the list.
         if (buildListView.Items.Count > 0)
            buildListView.Items[0].Selected = true;

         // Set default file set based on last one
         string fileSet = fileSetComboBox.Text;
         fileSetComboBox.Text = "";
         if (fileSetComboBox.Items.Count > 0)
         {
            fileSetComboBox.Text = fileSetComboBox.Items[0].ToString();
            if (fileSet.Length > 0)
            {
               for (int i=0; i<fileSetComboBox.Items.Count; i++)
               {
                  if (fileSetComboBox.Items[i].ToString() == fileSet)
                  {
                     fileSetComboBox.Text = fileSetComboBox.Items[i].ToString();
                     break;
                  }
               }
            }
         }

         // Get the list of Xbox's
         string xbox = xboxDropdown.Text;
         xboxDropdown.Text = "";
         try
         {
            XenonInterface.init();
            try
            {
               if (XenonInterface.mCurrentConsole != null)
               {
                  xboxDropdown.Text = XenonInterface.mXboxManager.DefaultConsole;
                  foreach (string name in XenonInterface.mXboxManager.Consoles)
                  {
                     if (!xboxDropdown.Items.Contains(name))
                        xboxDropdown.Items.Add(name);
                  }
               }
            }
            catch { }
            XenonInterface.destroy();
         }
         catch { }

         // Set default Xbox based on last one
         if (xboxDropdown.Items.Count > 0)
         {
            xboxDropdown.Text = xboxDropdown.Items[0].ToString();
            if (xbox.Length > 0)
            {
               for (int i = 0; i < xboxDropdown.Items.Count; i++)
               {
                  if (xboxDropdown.Items[i].ToString() == xbox)
                  {
                     xboxDropdown.Text = xboxDropdown.Items[i].ToString();
                     break;
                  }
               }
            }
         }

         mUpdateTimer = new System.Windows.Forms.Timer();
         mUpdateTimer.Interval = cUpdateInterval;
         mUpdateTimer.Start();
         mUpdateTimer.Tick += new EventHandler(update);

         enableButtons();
      }

      //============================================================================
      //============================================================================
      private void setupWorkPath()
      {
         // rg [2/1/06] - This is a bit of a hack - fix the working directory if it ends in "tools\XFS".
         // clm [3.18.06] - changed to use base directory to allow the editor to launch it w/ the proper directory.
         String path = AppDomain.CurrentDomain.BaseDirectory.ToLower();// Directory.GetCurrentDirectory();

         int toolPathIndex = path.LastIndexOf("\\code\\tools\\getbuild", path.Length - 1);
         if (toolPathIndex > -1)
         {
            String oldPath = path;
            path = path.Remove(toolPathIndex);
            path = path + "\\work";
            mWorkPath = path;
         }
         else
         {
            toolPathIndex = path.LastIndexOf("\\tools\\getbuild", path.Length - 1);
            if (toolPathIndex > -1)
            {
               String oldPath = path;
               path = path.Remove(toolPathIndex);
               mWorkPath = path;
            }
            else
            {
               // jce 10/19/2006 -- additional hack for dealing with "getbuild" in folder name instead of tools\getbuild
               toolPathIndex = path.LastIndexOf("\\getbuild", path.Length - 1);
               if (toolPathIndex > -1)
               {
                  String oldPath = path;
                  path = path.Remove(toolPathIndex);
                  mWorkPath = path;
               }
            }
         }
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
      private void loadFileSets()
      {
         try
         {
            XmlSerializer serializer = new XmlSerializer(typeof(FileSets));
            FileStream reader = new FileStream("filesets.xml", FileMode.Open, FileAccess.Read);
            mFileSets = (FileSets)serializer.Deserialize(reader);
            reader.Close();
         }
         catch { }
      }

      //============================================================================
      //============================================================================
      public void update(object sender, EventArgs eArgs)
      {
         if (sender == mUpdateTimer)
         {
            try
            {
               ArrayList events = getEventList();
               if (events.Count > 0)
               {
                  bool end = (logList.SelectedIndex == logList.Items.Count - 1);
                  foreach (string text in events)
                     log(text);
                  if (end)
                  {
                     int count = logList.Items.Count;
                     if (count > 0)
                     {
                        logList.SelectedIndex = count - 1;
                        //logList.SelectedIndex = -1;
                     }
                  }
                  events.Clear();
               }
            }
            catch
            {
            }

            if (mCopying)
            {
               if (mAutoLaunchDelay)
               {
                  if (Environment.TickCount - mCopyTime > 2000)
                  {
                     mAutoLaunchDelay = false;
                     mCopying = false;
                     enableButtons();
                     log("Completed copy.");
                     launchGame();
                  }
               }
               else if (!mCopyServerConnected)
               {
                  if (Server.hasActiveConnections())
                  {
                     mCopyServerConnected = true;
                     mCopyTime = Environment.TickCount;
                  }
                  else
                  {
                     if (Environment.TickCount - mCopyTime > 30000)
                     {
                        Server.shutdown();
                        mCopying = false;
                        enableButtons();
                        log("Cancelled.");
                     }
                  }
               }
               else
               {
                  if (!Server.hasActiveConnections())
                  {
                     if (Environment.TickCount - mCopyTime > 15000)
                     { 
                        registerCopyComplete();
                        Server.shutdown();
                        writeOptions();
                        if (mCopyAutoLaunch)
                        {
                           mAutoLaunchDelay = true;
                           mCopyTime = Environment.TickCount;
                        }
                        else
                        {
                           mCopying = false;
                           enableButtons();
                           log("Completed copy.");
                        }
                     }
                  }
               }
            }

            if (mCopyThread != null && mCopyComplete)
            {
               mCopyThread = null;
               if (!mCopyFailed && !mCopyCancel)
                  registerCopyComplete();
               writeOptions();
               if (mCopyAutoLaunch && !mCopyCancel && !mCopyFailed)
                  launchGame();
               enableButtons();
            }
         }
      }

      //============================================================================
      //============================================================================
      public static ArrayList getEventList()
      {
         lock (mCurrentEvents.SyncRoot)
         {
            ArrayList list = mCurrentEvents;
            if (mCurrentEvents == mEvents1)
               mCurrentEvents = mEvents2;
            else
               mCurrentEvents = mEvents1;
            return list;
         }
      }

      //============================================================================
      //============================================================================
      public static void asyncLog(string text)
      {
         lock (mCurrentEvents.SyncRoot)
         {
            mCurrentEvents.Add(text);
         }
      }

      //============================================================================
      //============================================================================
      private void resetOptions()
      {
         buildFolderTextBox.Text = @"";
         localFolderTextBox.Text = @"";
         preBatchFileTextBox.Text = @"";
         postBatchFileTextBox.Text = @"";
         configTextBox.Text = "";
         deleteFilesCheckBox.Checked = true;
         buildListView.Items.Clear();
         mBuildFiles.Clear();
         lastBuildTextBox.Text = @"";
         lastFileSetTextBox.Text = @"";
         lastFolderTextBox.Text = @"";
         lastXboxTextBox.Text = "";
         lastXboxFolderTextBox.Text = "";
         lastDeleteFilesCheckBox.Checked = false;
         lastSeparateFolderCheckBox.Checked = false;
         lastCopiedCheckBox.Checked = false;
      }

      //============================================================================
      //============================================================================
      private void readOptions()
      {
         if (mOptionsLocked)
            return;

         mOptionsLocked = true;

         resetOptions();

         try
         {
            XmlSerializer serializer = new XmlSerializer(typeof(Settings));
            FileStream reader = new FileStream("settings.xml", FileMode.Open, FileAccess.Read);
            mSettings = (Settings)serializer.Deserialize(reader);
            reader.Close();
         }
         catch
         {
            mSettings = new Settings();
         }

         buildFolderTextBox.Text = mSettings.mBuildFolder;
         localFolderTextBox.Text = mSettings.mLocalFolder;
         preBatchFileTextBox.Text = mSettings.mPreBatchFile;
         postBatchFileTextBox.Text = mSettings.mPostBatchFile;
         xboxDropdown.Text = mSettings.mXboxName;
         xboxFolderTextBox.Text = mSettings.mXboxFolder;
         fileSetComboBox.Text = mSettings.mFileSet;
         deleteFilesCheckBox.Checked = true;//mSettings.mDeleteFiles;
         separateFolderCheckBox.Checked = mSettings.mSeparateFolder;
         copyMethodDropdown.Text = mSettings.mCopyMethod;

         if (ipDropdown.Items.Contains(mSettings.mServerIP))
            ipDropdown.Text = mSettings.mServerIP;

         if (exeDropdown.Items.Contains(mSettings.mExe))
            exeDropdown.Text = mSettings.mExe;

         xfsCheckBox.Checked = mSettings.mXFS;

         configTextBox.Text = "";
         if (mSettings.mConfigs.Count > 0)
         {
            string configs = "";
            foreach (string config in mSettings.mConfigs)
            {
               if (config != "")
                  configs += config + Environment.NewLine;
            }
            configTextBox.Text = configs;
         }

         if (mSettings.mGets.Count > 0)
         {
            bool first = true;
            foreach (SettingsGet settingsGet in mSettings.mGets)
            {
               string history = settingsGet.mBuild + ", " + settingsGet.mFileSet + ", " + settingsGet.mFolder;
               historyDropdown.Items.Add(history);
               if (first)
               {
                  first = false;
                  historyDropdown.Text = history;
               }
               else if (settingsGet.mBuild == mSettings.mLastBuild && settingsGet.mFileSet == mSettings.mLastFileSet && settingsGet.mFolder == mSettings.mLastFolder)
                  historyDropdown.Text = history;
            }
         }

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

         mSettings.mBuildFolder = buildFolderTextBox.Text;
         mSettings.mLocalFolder = localFolderTextBox.Text;
         mSettings.mPreBatchFile = preBatchFileTextBox.Text;
         mSettings.mPostBatchFile = postBatchFileTextBox.Text;
         mSettings.mXboxName = xboxDropdown.Text;
         mSettings.mXboxFolder = xboxFolderTextBox.Text;
         mSettings.mFileSet = fileSetComboBox.Text;
         mSettings.mDeleteFiles = deleteFilesCheckBox.Checked;
         mSettings.mSeparateFolder = separateFolderCheckBox.Checked;
         mSettings.mLastBuild = lastBuildTextBox.Text;
         mSettings.mLastFileSet = lastFileSetTextBox.Text;
         mSettings.mLastFolder = lastFolderTextBox.Text;
         mSettings.mCopyMethod = copyMethodDropdown.Text;
         mSettings.mServerIP = ipDropdown.Text;
         mSettings.mExe = exeDropdown.Text;
         mSettings.mXFS = xfsCheckBox.Checked;

         mSettings.mConfigs.Clear();
         if (configTextBox.Lines.Length > 0)
         {
            foreach (string config in configTextBox.Lines)
               mSettings.mConfigs.Add(config);
         }

         try
         {
            XmlSerializer serializer = new XmlSerializer(typeof(Settings));
            TextWriter writer = new StreamWriter("settings.xml");
            serializer.Serialize(writer, mSettings);
            writer.Close();
         }
         catch
         {
         }
      }

      //============================================================================
      //============================================================================
      private void getFileSets()
      {
         fileSetComboBox.Items.Clear();


         foreach (FileSet fileSet in mFileSets.mFileSets)
            fileSetComboBox.Items.Add(fileSet.mName);
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
      private void pickPreBatchFile()
      {
         string dir = @"";
         if (preBatchFileTextBox.Text.Length > 0)
            dir = Path.GetDirectoryName(preBatchFileTextBox.Text);
         else
            dir = Directory.GetCurrentDirectory();
         OpenFileDialog dialog = new OpenFileDialog();
         dialog.InitialDirectory = dir;
         dialog.Filter = "Batch files (*.bat)|*.bat";
         dialog.FilterIndex = 1;
         dialog.RestoreDirectory = true;
         if (dialog.ShowDialog() == DialogResult.OK)
         {
            preBatchFileTextBox.Text = dialog.FileName;
            writeOptions();
         }
      }

      //============================================================================
      //============================================================================
      private void pickPostBatchFile()
      {
         string dir = @"";
         if (postBatchFileTextBox.Text.Length > 0)
            dir = Path.GetDirectoryName(postBatchFileTextBox.Text);
         else
            dir = Directory.GetCurrentDirectory();
         OpenFileDialog dialog = new OpenFileDialog();
         dialog.InitialDirectory = dir;
         dialog.Filter = "Batch files (*.bat)|*.bat";
         dialog.FilterIndex = 1;
         dialog.RestoreDirectory = true;
         if (dialog.ShowDialog() == DialogResult.OK)
         {
            postBatchFileTextBox.Text = dialog.FileName;
            writeOptions();
         }
      }

      //============================================================================
      //============================================================================
      private void doGetBuild(bool autoCopyAndLaunch, bool autoEmulateAndLaunch)
      {
         if (mProcessCaller != null)
            return;

         if (buildListView.Items.Count == 0 || buildListView.SelectedIndices.Count == 0)
         {
            log("ERROR: No build selected.");
            return;
         }

         // Get the path to 7z.exe and make sure it exists
         string zipPath = Directory.GetCurrentDirectory();//AppDomain.CurrentDomain.BaseDirectory;
         if (zipPath != "")
         {
            string c = zipPath.Substring(zipPath.Length - 1);
            if (c != @"\")
               zipPath += @"\";
         }
         zipPath += "7-Zip\\";
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
            if (c != @"\")
               buildPath += @"\";
         }

         // Get the file set
         loadFileSets();
         string fileSet = fileSetComboBox.Text;

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
            if (c != @"\")
               localPath += @"\";
         }

         if (separateFolderCheckBox.Checked)
         {
            string buildName = buildFile;
            string ext = buildName.Substring(buildName.Length - 3);
            if (ext.ToLower() == ".7z")
               buildName = buildName.Substring(0, buildName.Length - 3);
            localPath += buildName + @"\";
         }

         disableButtons();

         log("Getting build " + buildFile + " (" + fileSet + ")...");

         lastBuildTextBox.Text = buildFile;
         lastFileSetTextBox.Text = fileSet;
         lastFolderTextBox.Text = localPath;
         lastXboxTextBox.Text = "";
         lastXboxFolderTextBox.Text = "";
         lastDeleteFilesCheckBox.Checked = deleteFilesCheckBox.Checked;
         lastSeparateFolderCheckBox.Checked = separateFolderCheckBox.Checked;
         lastCopiedCheckBox.Checked = false;

         // Remove any old gets that are no longer valid since they are being overwritten
         string newFolder = Path.GetDirectoryName(localPath).ToLower();
         ArrayList deprecatedGets = new ArrayList();
         int index = 0;
         foreach (SettingsGet settingsGet in mSettings.mGets)
         {
            string oldFolder = Path.GetDirectoryName(settingsGet.mFolder).ToLower();
            if (newFolder.Length <= oldFolder.Length)
            {
               if (oldFolder.Substring(0, newFolder.Length) == newFolder)
                  deprecatedGets.Insert(0, index);
            }
            index++;
         }
         foreach (int oldIndex in deprecatedGets)
         {
            mSettings.mGets.RemoveAt(oldIndex);
            if (historyDropdown.Items.Count > oldIndex)
               historyDropdown.Items.RemoveAt(oldIndex);
         }

         SettingsGet newGet = new SettingsGet();
         newGet.mBuild = buildFile;
         newGet.mFileSet = fileSet;
         newGet.mFolder = localPath;
         newGet.mSeparateFolder = separateFolderCheckBox.Checked;
         newGet.mDeleteFiles = deleteFilesCheckBox.Checked;
         mSettings.mGets.Insert(0, newGet);
         string history = newGet.mBuild + ", " + newGet.mFileSet + ", " + newGet.mFolder;
         historyDropdown.Items.Insert(0, history);
         historyDropdown.Text = history;

         // Clear out the local folder
         if (deleteFilesCheckBox.Checked)
            deleteLocalFiles(localPath);

         // Call the preBatch file
         if (preBatchFileTextBox.Text != @"")
         {
            log("Executing " + preBatchFileTextBox.Text + "...");

            Process preBatchFileProcess = new Process();
            preBatchFileProcess.StartInfo.FileName = preBatchFileTextBox.Text;
            preBatchFileProcess.StartInfo.WindowStyle = ProcessWindowStyle.Normal;
            //preBatchFileProcess.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
            preBatchFileProcess.StartInfo.UseShellExecute = true;
            preBatchFileProcess.Start();

            preBatchFileProcess.WaitForExit();
            log("Done with " + preBatchFileTextBox.Text);
         }

         // Create the fileList.txt file
         string fileListTxt = Directory.GetCurrentDirectory();
         if (fileListTxt != "")
         {
            string c = fileListTxt.Substring(fileListTxt.Length - 1);
            if (c != @"\")
               fileListTxt += @"\";
         }
         fileListTxt += "fileList.txt";
         try
         {
            if (File.Exists(fileListTxt))
               File.Delete(fileListTxt);
         }
         catch { }
         try
         {
            TextWriter writer = new StreamWriter(fileListTxt);
            foreach (FileSet fileSetItem in mFileSets.mFileSets)
            {
               if (fileSetItem.mName == fileSet)
               {
                  if (fileSetItem.mBaseFiles)
                  {
                     foreach (string fileName in mFileSets.mBaseFiles)
                        writer.WriteLine(fileName);
                  }
                  if (fileSetItem.mVideoFiles)
                  {
                     foreach (string fileName in mFileSets.mVideoFiles)
                        writer.WriteLine(fileName);
                  }
                  if (fileSetItem.mCampaignFiles)
                  {
                     foreach (string fileName in mFileSets.mCampaignFiles)
                        writer.WriteLine(fileName);
                  }
                  if (fileSetItem.mSkirmishFiles)
                  {
                     foreach (string fileName in mFileSets.mSkirmishFiles)
                        writer.WriteLine(fileName);
                  }
                  foreach (string fileName in fileSetItem.mCustomFiles)
                     writer.WriteLine(fileName);
               }
            }
            writer.Close();
         }
         catch { }

         // Run 7z to decompress the build
         mProcessType = ProcessType.cGet;
         mProcessAutoCopyAndLaunch = autoCopyAndLaunch;
         mProcessAutoEmulateAndLaunch = autoEmulateAndLaunch;
         mProcessCaller = new ProcessCaller(this);
         mProcessCaller.FileName = zipPath + @"7z.exe";
         mProcessCaller.Arguments = @"x " + buildPath + buildFile + @" -y -o" + localPath;
         if (fileSet != "")
            mProcessCaller.Arguments += @" -i@fileList.txt";
         mProcessCaller.StdErrReceived += new DataReceivedHandler(writeProcessOutput);
         mProcessCaller.StdOutReceived += new DataReceivedHandler(writeProcessOutput);
         mProcessCaller.Completed += new EventHandler(processCompleted);
         mProcessCaller.Cancelled += new EventHandler(processCancelled);
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
      private void disableButtons()
      {
         goButton.Enabled = false;
         getCopyLaunchButton.Enabled = false;
         cancelButton.Enabled = true;

         copyButton.Enabled = false ;
         launchButton.Enabled = false;
         rebootButton.Enabled = false;
         deleteLocalButton.Enabled = false;
         deleteXboxButton.Enabled = false;

         historyDropdown.Enabled = false;

         this.Cursor = Cursors.AppStarting;
      }

      //============================================================================
      //============================================================================
      private void enableButtons()
      {
         if (buildListView.SelectedIndices.Count > 0)
         {
            goButton.Enabled = true;
            getCopyLaunchButton.Enabled = true;
         }

         cancelButton.Enabled = false;

         if (lastBuildTextBox.Text != @"")
         {
            copyButton.Enabled = true;
            deleteLocalButton.Enabled = true;
            if (lastCopiedCheckBox.Checked)
            {
               launchButton.Enabled = true;
               deleteXboxButton.Enabled = true;
            }
         }

         if (xboxDropdown.Text != "")
            rebootButton.Enabled = true;

         historyDropdown.Enabled = true;

         this.Cursor = Cursors.Default;
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

         if (mProcessType == ProcessType.cXfsCopy)
         {
            mCopying = true;
            mAutoLaunchDelay = false;
            mCopyTime = Environment.TickCount;
            return;
         }

         if (mProcessType == ProcessType.cGet && postBatchFileTextBox.Text != @"")
         {
            log("Completed get.");
            log("Executing " + postBatchFileTextBox.Text + "...");
            mProcessType = ProcessType.cBatch;
            mProcessCaller = new ProcessCaller(this);
            mProcessCaller.FileName = postBatchFileTextBox.Text;
            mProcessCaller.Arguments = @"";
            mProcessCaller.StdErrReceived += new DataReceivedHandler(writeProcessOutput);
            mProcessCaller.StdOutReceived += new DataReceivedHandler(writeProcessOutput);
            mProcessCaller.Completed += new EventHandler(processCompleted);
            mProcessCaller.Cancelled += new EventHandler(processCancelled);
            //mProcessCaller.Failed += no event handler for this one, yet.
            mProcessCaller.Start();
            return;
         }

         if (mProcessType == ProcessType.cDeleteBeforeInternalCopy)
         {
            log("Copying...");
            mCopyThread = new Thread(new ThreadStart(doCopyBuild));
            mCopyThread.IsBackground = true;
            mCopyThread.Start();
            return;
         }

         if (mProcessType == ProcessType.cDeleteBeforeXbcpCopy)
         {
            log("Copying...");
            string xbcpPath = @"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\xbcp.exe";
            mProcessType = ProcessType.cXbcpCopy;
            mProcessCaller = new ProcessCaller(this);
            mProcessCaller.FileName = xbcpPath;
            mProcessCaller.Arguments = @"/R /S /H /Y /T /F " + mCopyLocalFolder + @" " + mCopyXboxFolder;
            mProcessCaller.StdErrReceived += new DataReceivedHandler(writeProcessOutput);
            mProcessCaller.StdOutReceived += new DataReceivedHandler(writeProcessOutput);
            mProcessCaller.Completed += new EventHandler(processCompleted);
            mProcessCaller.Cancelled += new EventHandler(processCancelled);
            mProcessCaller.Start();
            return;
         }

         if (mProcessType == ProcessType.cXbcpCopy)
         {
            int index = historyDropdown.SelectedIndex;
            if (index != -1 && index < mSettings.mGets.Count)
            {
               SettingsGet settingsGet = mSettings.mGets[index];
               if (settingsGet.mCopies.Count > 0)
               {
                  foreach (SettingsCopy settingsCopy in settingsGet.mCopies)
                  {
                     if (settingsCopy.mXbox == mCopyXboxName)
                     {
                        settingsGet.mCopies.Remove(settingsCopy);
                        break;
                     }
                  }
               }

               SettingsCopy newCopy = new SettingsCopy();
               newCopy.mXbox = mCopyXboxName;
               newCopy.mFolder = mCopyXboxFolder;
               newCopy.mCopied = true;
               settingsGet.mCopies.Add(newCopy);

               updateLastXboxCopyInfo();
            }

            writeOptions();

            log("Completed copy.");

            if (mCopyAutoLaunch)
               launchGame();

            enableButtons();
            return;
         }

         enableButtons();

         writeOptions();

         if ((mProcessType == ProcessType.cGet || mProcessType == ProcessType.cBatch))
         {
            if (mProcessType == ProcessType.cGet)
               log("Completed get.");
            if (mProcessAutoCopyAndLaunch)
               copyBuild(true);
            else if (mProcessAutoEmulateAndLaunch)
               emulateBuild(true);
            return;
         }
      }

      //============================================================================
      //============================================================================
      private void processCancelled(object sender, EventArgs e)
      {
         if (mProcessType == ProcessType.cGet)
         {
            lastBuildTextBox.Text = @"";
            lastFileSetTextBox.Text = @"";
            lastFolderTextBox.Text = @"";

            if (mSettings.mGets.Count > 0)
               mSettings.mGets.RemoveAt(0);

            historyDropdown.Text = "";
            if (historyDropdown.Items.Count > 0)
               historyDropdown.Items.RemoveAt(0);
         }
         else if (mProcessType == ProcessType.cInternalCopy)
            lastCopiedCheckBox.Checked = false;
         else if (mProcessType == ProcessType.cXbcpCopy)
         {
            lastCopiedCheckBox.Checked = false;
            lastXboxTextBox.Text = "";
            lastXboxFolderTextBox.Text = "";
         }
         else if (mProcessType == ProcessType.cXfsCopy)
         {
            Server.shutdown();
            mCopying = false;
         }

         mProcessCaller = null;

         enableButtons();

         writeOptions();

         log("Cancelled.");
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
               /*
               string name = fileInfo.Name.ToLower();
               if (name == "user.cfg" || name == "xfs.txt")
               {
                  empty = false;
                  continue;
               }
               */
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
      private bool deleteXboxFiles(string xboxName, string xboxFolder, ProcessType processType)
      {
         string xbdelPath = @"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\xbdel.exe";
         bool xbdelExists = false;
         try
         {
            xbdelExists = File.Exists(xbdelPath);
         }
         catch { }
         if (!xbdelExists)
         {
            log("ERROR: Can't find xbdel program.");
            return false;
         }

         string xboxPath = xboxFolder;
         string c = xboxPath.Substring(xboxPath.Length - 1);
         if (c == @"\")
            xboxPath = xboxPath.Substring(0, xboxPath.Length - 1);

         mProcessType = processType;
         mProcessCaller = new ProcessCaller(this);
         mProcessCaller.FileName = xbdelPath;
         mProcessCaller.Arguments = @"/R /S /H /F /X:" + xboxName + " " + xboxPath;
         mProcessCaller.StdErrReceived += new DataReceivedHandler(writeProcessOutput);
         mProcessCaller.StdOutReceived += new DataReceivedHandler(writeProcessOutput);
         mProcessCaller.Completed += new EventHandler(processCompleted);
         mProcessCaller.Cancelled += new EventHandler(processCancelled);
         mProcessCaller.Start();

         return true;
      }

      //============================================================================
      //============================================================================
      private void copyBuild(bool autoLaunch)
      {
         writeLocalFiles();

         if (copyMethodDropdown.Text == "Internal")
            internalCopyBuild(autoLaunch);
         else if (copyMethodDropdown.Text == "xfsCopy")
            xfsCopyBuild(autoLaunch);
         else 
            xbcpCopyBuild(autoLaunch);
      }

      //============================================================================
      //============================================================================
      private void internalCopyBuild(bool autoLaunch)
      {
         mCopyLocalFolder = lastFolderTextBox.Text;
         mCopyXboxFolder = xboxFolderTextBox.Text;
         mCopySeparateFolder = lastSeparateFolderCheckBox.Checked;
         mCopyXboxName = xboxDropdown.Text;
         mCopyCancel = false;
         mCopyComplete = false;
         mCopyFailed = false;
         mCopyAutoLaunch = autoLaunch;

         string buildFile = lastBuildTextBox.Text;
         if (buildFile == "")
         {
            asyncLog("ERROR: No build to copy.");
            return;
         }

         if (mCopyLocalFolder.Length == 0)
         {
            asyncLog("ERROR: No local folder specified.");
            return;
         }
         else
         {
            string c = mCopyLocalFolder.Substring(mCopyLocalFolder.Length - 1);
            if (c != @"\")
               mCopyLocalFolder += @"\";
         }

         if (mCopyXboxFolder.Length == 0)
         {
            asyncLog("ERROR: No xbox folder specified.");
            return;
         }
         else
         {
            string c = mCopyXboxFolder.Substring(mCopyXboxFolder.Length - 1);
            if (c != @"\")
               mCopyXboxFolder += @"\";
         }
         if (mCopySeparateFolder)
         {
            string buildName = buildFile;
            string ext = buildName.Substring(buildName.Length - 3);
            if (ext.ToLower() == ".7z")
               buildName = buildName.Substring(0, buildName.Length - 3);
            mCopyXboxFolder += buildName + @"\";
         }

         disableButtons();

         if (!deleteXboxFiles(mCopyXboxName, mCopyXboxFolder, ProcessType.cDeleteBeforeInternalCopy))
            enableButtons();

         //mCopyThread = new Thread(new ThreadStart(doCopyBuild));
         //mCopyThread.IsBackground = true;
         //mCopyThread.Start();
      }

      //============================================================================
      //============================================================================
      public static void doCopyBuild()
      {
         recursiveCopy(mCopyLocalFolder, mCopyXboxFolder);

         if (!mCopyFailed)
            asyncLog("Completed.");

         mCopyComplete = true;
      }

      //============================================================================
      //============================================================================
      public static void recursiveCopy(string localPath, string xboxPath)
      {
         string searchPattern = @"*.*";
         FileInfo[] fileInfoList = null;
         DirectoryInfo[] dirInfoList = null;
         try
         {
            DirectoryInfo dirInfo = new DirectoryInfo(localPath);
            dirInfoList = dirInfo.GetDirectories();
            fileInfoList = dirInfo.GetFiles(searchPattern);
         }
         catch { }
         if (dirInfoList != null)
         {
            foreach (DirectoryInfo dirInfo in dirInfoList)
            {
               string newLocalPath = localPath + dirInfo.Name + "\\";
               string newXboxPath = xboxPath + dirInfo.Name + "\\";
               recursiveCopy(newLocalPath, newXboxPath);
               if (mCopyFailed)
                  return;
               if (mCopyCancel)
               {
                  asyncLog("Copy cancelled.");
                  mCopyFailed = true;
                  return;
               }
            }
         }
         if (fileInfoList != null)
         {
            foreach (FileInfo fileInfo in fileInfoList)
            {
               try
               {
                  if (mCopyFailed)
                     return;
                  if (mCopyCancel)
                  {
                     asyncLog("Copy cancelled.");
                     mCopyFailed = true;
                     return;
                  }

                  string source = localPath + fileInfo.Name;
                  string dest = xboxPath + fileInfo.Name;

                  asyncLog("Copying " + fileInfo.Name + "...");

                  XenonInterface.init();
                  XenonInterface.eErrorType retval = XenonInterface.sendFile(mCopyXboxName, source, dest, true, false);
                  XenonInterface.destroy();
                  if (retval != XenonInterface.eErrorType.cOK)
                  {
                     asyncLog("ERROR: File copy failed.");
                     mCopyFailed = true;
                     return;
                  }

               }
               catch
               {
                  asyncLog("ERROR: Unable to copy files.");
                  mCopyFailed = true;
                  return;
               }
            }
         }
      }

      //============================================================================
      //============================================================================
      private void xbcpCopyBuild(bool autoLaunch)
      {
         string buildFile = (String)lastBuildTextBox.Text;
         if (buildFile == "")
         {
            log("ERROR: No build to copy.");
            return;
         }

         string xbcpPath = @"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\xbcp.exe";
         bool xbcpExists = false;
         try
         {
            xbcpExists = File.Exists(xbcpPath);
         }
         catch { }
         if (!xbcpExists)
         {
            log("ERROR: Can't find xbcp program.");
            return;
         }

         string localPath = localFolderTextBox.Text;
         if (localPath.Length == 0)
         {
            log("ERROR: No local folder specified.");
            return;
         }
         else
         {
            string c = localPath.Substring(localPath.Length - 1);
            if (c == @"\")
               localPath = localPath.Substring(0, localPath.Length - 1);
         }

         string xboxPath = xboxFolderTextBox.Text;

         if (lastSeparateFolderCheckBox.Checked)
         {
            string buildName = buildFile;
            string ext = buildName.Substring(buildName.Length - 3);
            if (ext.ToLower() == ".7z")
               buildName = buildName.Substring(0, buildName.Length - 3);
            localPath += @"\" + buildName;
            xboxPath += @"\" + buildName;
         }

         disableButtons();

         mCopyLocalFolder = localPath;
         mCopyXboxName = xboxDropdown.Text;
         mCopyXboxFolder = xboxPath;
         mCopySeparateFolder = lastSeparateFolderCheckBox.Checked;
         mCopyAutoLaunch = autoLaunch;

         if (!deleteXboxFiles(mCopyXboxName, mCopyXboxFolder, ProcessType.cDeleteBeforeXbcpCopy))
            enableButtons();
      }

      //============================================================================
      //============================================================================
      private void xfsCopyBuild(bool autoLaunch)
      {
         string buildFile = (String)lastBuildTextBox.Text;
         if (buildFile == "")
         {
            log("ERROR: No build to copy.");
            return;
         }

         if (ipDropdown.Text == "")
         {
            log("ERROR: Server IP address not set.");
            return;
         }

         string xbcpPath = @"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\xbcp.exe";
         bool xbcpExists = false;
         try
         {
            xbcpExists = File.Exists(xbcpPath);
         }
         catch { }
         if (!xbcpExists)
         {
            log("ERROR: Can't find xbcp program.");
            return;
         }

         log("Copying...");

         if (!Server.setup(null))
         {
            log("ERROR: Failed to initialize xfsCopy.");
            return;
         }

         string localPath = localFolderTextBox.Text;
         if (localPath.Length == 0)
         {
            log("ERROR: No local folder specified.");
            return;
         }
         else
         {
            string c = localPath.Substring(localPath.Length - 1);
            if (c != @"\")
               localPath += @"\";
         }

         string xboxPath = xboxFolderTextBox.Text;
         if (xboxPath.Length == 0)
         {
            log("ERROR: No xbox folder specified.");
            return;
         }
         else
         {
            string c = xboxPath.Substring(xboxPath.Length - 1);
            if (c != @"\")
               xboxPath += @"\";
         }

         if (lastSeparateFolderCheckBox.Checked)
         {
            string buildName = buildFile;
            string ext = buildName.Substring(buildName.Length - 3);
            if (ext.ToLower() == ".7z")
               buildName = buildName.Substring(0, buildName.Length - 3);
            localPath += buildName + @"\";
            xboxPath += buildName + @"\";
         }

         try
         {
            String path = Directory.GetCurrentDirectory() + "\\xfsCopy\\xfsCopy.txt";
            TextWriter writer = new StreamWriter(path);
            writer.WriteLine("xbox " + ipDropdown.Text);
            writer.WriteLine("sourcePath " + localPath + "*.*");
            writer.WriteLine("destPath game:\\");
            writer.Close();
         }
         catch 
         {
            Server.shutdown();
            log("ERROR: Failed to write xfsCopy.txt file");
            return;
         }

         string xboxName = xboxDropdown.Text;

         try
         {
            String path = Directory.GetCurrentDirectory() + "\\xfsCopy\\xfsCopy.bat";
            TextWriter writer = new StreamWriter(path);
            writer.WriteLine("\"C:\\Program Files\\Microsoft Xbox 360 SDK\\bin\\win32\\xbdel\" /R /S /H /F /X:" + xboxName + " " + xboxPath);
            writer.WriteLine("\"C:\\Program Files\\Microsoft Xbox 360 SDK\\bin\\win32\\xbcp\" /F /T /Y /R /X:" + xboxName + " " + Directory.GetCurrentDirectory() + "\\xfsCopy\\*.* " + xboxPath);
            writer.WriteLine("\"C:\\Program Files\\Microsoft Xbox 360 SDK\\bin\\win32\\xbreboot\" /X:" + xboxName + " " + xboxPath + "xfsCopy.xex");
            writer.Close();
         }
         catch
         {
            Server.shutdown();
            log("ERROR: Failed to write xfsCopy.bat file");
            return;
         }

         Server.start();

         disableButtons();

         mCopyLocalFolder = localPath;
         mCopyXboxName = xboxDropdown.Text;
         mCopyXboxFolder = xboxPath;
         mCopySeparateFolder = lastSeparateFolderCheckBox.Checked;
         mCopyAutoLaunch = autoLaunch;

         mProcessType = ProcessType.cXfsCopy;
         mProcessCaller = new ProcessCaller(this);
         mProcessCaller.FileName = Directory.GetCurrentDirectory() + "\\xfsCopy\\xfsCopy.bat";
         mProcessCaller.Arguments = "";
         mProcessCaller.StdErrReceived += new DataReceivedHandler(writeProcessOutput);
         mProcessCaller.StdOutReceived += new DataReceivedHandler(writeProcessOutput);
         mProcessCaller.Completed += new EventHandler(processCompleted);
         mProcessCaller.Cancelled += new EventHandler(processCancelled);
         mProcessCaller.Start();
      }

      //============================================================================
      //============================================================================
      void registerCopyComplete()
      {
         int index = historyDropdown.SelectedIndex;
         if (index != -1 && index < mSettings.mGets.Count)
         {
            SettingsGet settingsGet = mSettings.mGets[index];
            if (settingsGet.mCopies.Count > 0)
            {
               foreach (SettingsCopy settingsCopy in settingsGet.mCopies)
               {
                  if (settingsCopy.mXbox == mCopyXboxName)
                  {
                     settingsGet.mCopies.Remove(settingsCopy);
                     break;
                  }
               }
            }
            SettingsCopy newCopy = new SettingsCopy();
            newCopy.mXbox = mCopyXboxName;
            newCopy.mFolder = mCopyXboxFolder;
            newCopy.mCopied = true;
            settingsGet.mCopies.Add(newCopy);
            updateLastXboxCopyInfo();
         }
      }

      //============================================================================
      //============================================================================
      private void emulateBuild(bool autoLaunch)
      {
         string buildFile = (String)lastBuildTextBox.Text;
         if (buildFile == "")
         {
            log("ERROR: No build to copy.");
            return;
         }

         if (ipDropdown.Text == "")
         {
            log("ERROR: Server IP address not set.");
            return;
         }

         string xbEmulate = @"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\xbEmulate.exe";
         bool xbEmulateExists = false;
         try
         {
            xbEmulateExists = File.Exists(xbEmulate);
         }
         catch { }
         if (!xbEmulateExists)
         {
            log("ERROR: Can't find xbEmulate program.");
            return;
         }

         log("Copying...");

         if (!Server.setup(null))
         {
            log("ERROR: Failed to initialize xfsCopy.");
            return;
         }

         string localPath = localFolderTextBox.Text;
         if (localPath.Length == 0)
         {
            log("ERROR: No local folder specified.");
            return;
         }
         else
         {
            string c = localPath.Substring(localPath.Length - 1);
            if (c != @"\")
               localPath += @"\";
         }

         string buildName = buildFile;
         string ext = buildName.Substring(buildName.Length - 3);
         if (ext.ToLower() == ".7z")
            buildName = buildName.Substring(0, buildName.Length - 3);

         if (lastSeparateFolderCheckBox.Checked)
            localPath += buildName + @"\";

         try
         {
            String path = Directory.GetCurrentDirectory() + "\\" + buildName + ".XGD";
            TextWriter writer = new StreamWriter(path);

            writer.WriteLine("<XboxGameDiscLayout>");
            writer.WriteLine("  <Header>");
            writer.WriteLine("    <Version Type=\"schema\" Major=\"1\" Minor=\"1\"/>");
            writer.WriteLine("    <Version Type=\"compat\" Major=\"1\" Minor=\"0\"/>");
            writer.WriteLine("    <LastSave DateTime=\"0\">");
            writer.WriteLine("      <Tool ID=\"GDEngine\">");
            writer.WriteLine("        <Version Type=\"app\" Major=\"1\" Minor=\"16\" Build=\"7776\" QFE=\"0\"/>");
            writer.WriteLine("      </Tool>");
            writer.WriteLine("    </LastSave>");
            writer.WriteLine("  </Header>");
            writer.WriteLine("  <Sources>");
            writer.WriteLine("    <Source Id=\"0\" PCSrcDirectory=\"C:\\x\\build\\build_968\\\" SearchPattern=\"*.*\" XboxMediaDirectory=\"\\\" Recurse=\"Yes\"/>");
            writer.WriteLine("  </Sources>");
            writer.WriteLine("  <Disc Id=\"0\" Type=\"0\">");
            writer.WriteLine("    <Files Location=\"layer0\" Sectors=\"1783936\">");
            writer.WriteLine("      <Object Type=\"Volume\"  LBA=\"32\" Blocks=\"2\"/>");
            writer.WriteLine("      <Object Type=\"Reserved\"  LBA=\"48\" Blocks=\"4096\"/>");
            writer.WriteLine("      <Object Type=\"File\"       Root=\"0\" SubDirectory=\"\\\" Name=\"xgameP.xdb\" Size=\"10632200\" LBA=\"1731608\" Blocks=\"5192\"/>");
            writer.WriteLine("      <Object Type=\"Directory\"  Root=\"0\" SubDirectory=\"&lt;ROOT&gt;\" Name=\"\" Size=\"2048\" LBA=\"1736800\" Blocks=\"1\"/>");
            writer.WriteLine("      <Object Type=\"File\"       Root=\"0\" SubDirectory=\"\\\" Name=\"xgameP.xex\" Size=\"19742720\" LBA=\"1736801\" Blocks=\"9641\"/>");
            writer.WriteLine("      <Object Type=\"File\"       Root=\"0\" SubDirectory=\"\\\" Name=\"civUNSC.era\" Size=\"17367040\" LBA=\"1746442\" Blocks=\"8480\"/>");
            writer.WriteLine("    </Files>");
            writer.WriteLine("    <Files Location=\"layer1\" Sectors=\"1783936\">");
            writer.WriteLine("    </Files>");
            writer.WriteLine("    <Files Location=\"scratch\">");
            writer.WriteLine("    </Files>");
            writer.WriteLine("  </Disc>");
            writer.WriteLine("</XboxGameDiscLayout>");
            writer.Close();
         }
         catch
         {
            Server.shutdown();
            log("ERROR: Failed to write XGD file");
            return;
         }

         string xboxName = xboxDropdown.Text;

         try
         {
            String path = Directory.GetCurrentDirectory() + "\\getBuildEmulate.bat";
            TextWriter writer = new StreamWriter(path);
            writer.WriteLine("\"C:\\Program Files\\Microsoft Xbox 360 SDK\\bin\\win32\\xbEmulate\" /F /T /Y /R /X:" + xboxName + " " + localPath);
            writer.WriteLine("\"C:\\Program Files\\Microsoft Xbox 360 SDK\\bin\\win32\\xbreboot\" /X:" + xboxName);
            writer.Close();
         }
         catch
         {
            Server.shutdown();
            log("ERROR: Failed to write getBuildEmulate.bat file");
            return;
         }

         mProcessType = ProcessType.cEmulate;
         mProcessCaller = new ProcessCaller(this);
         mProcessCaller.FileName = Directory.GetCurrentDirectory() + "\\getBuildEmulate.bat";
         mProcessCaller.Arguments = "";
         mProcessCaller.StdErrReceived += new DataReceivedHandler(writeProcessOutput);
         mProcessCaller.StdOutReceived += new DataReceivedHandler(writeProcessOutput);
         mProcessCaller.Completed += new EventHandler(processCompleted);
         mProcessCaller.Cancelled += new EventHandler(processCancelled);
         mProcessCaller.Start();
      }

      //============================================================================
      //============================================================================
      private void launchGame()
      {
         string xboxName = lastXboxTextBox.Text;
         if (xboxName == "")
         {
            log("ERROR: Xbox not selected.");
            return;
         }

         string xboxPath = lastXboxFolderTextBox.Text;
         if (xboxPath == "")
         {
            log("ERROR: No build to launch.");
            return;
         }
         else
         {
            string c = xboxPath.Substring(xboxPath.Length - 1);
            if (c != @"\")
               xboxPath += @"\";
         }

         string gameExe = xboxPath + exeDropdown.Text + ".xex";

         writeUserCfg(true);
         writeXfsTxt(true);

         if (xfsCheckBox.Checked)
         {
            bool useXfs = false;
            try
            {
               if (!XFSInterface.launchApp())
               {
                  log("ERROR: Could not launch XFS.");
                  return;
               }
               useXfs = true;
            }
            catch 
            {
            }
            if (!useXfs)
            {
               log("ERROR: Could not launch XFS.");
               return;
            }
         }

         try
         {
            XenonInterface.init();
            if (XenonInterface.mXboxManager != null)
            {
               XenonInterface.reboot(xboxName, gameExe, xboxPath, "", XboxRebootFlags.Title);
               log("Launched " + gameExe + ".");
            }
            XenonInterface.destroy();
         }
         catch
         {
            log("ERROR: Unable to launch " + gameExe + ".");
         }
      }

      //============================================================================
      //============================================================================
      private bool writeUserCfg(bool sendToDevKit)
      {
         string path = lastFolderTextBox.Text;
         if (path.Length == 0)
            return false;
         else
         {
            string c = path.Substring(path.Length - 1);
            if (c != @"\")
               path += @"\";
         }

         string userCfg = Directory.GetCurrentDirectory();
         if (userCfg != "")
         {
            string c = userCfg.Substring(userCfg.Length - 1);
            if (c != @"\")
               userCfg += @"\";
         }
         userCfg += "user.cfg";

         try
         {
            if (File.Exists(userCfg))
               File.Delete(userCfg);
         }
         catch { }
         try
         {
            TextWriter writer = new StreamWriter(userCfg);
            foreach (string config in configTextBox.Lines)
               writer.WriteLine(config);
            writer.Close();
         }
         catch { }

         try
         {
            Directory.CreateDirectory(path + "startup");
         }
         catch { }

         try
         {
            File.Copy(userCfg, path + "startup\\user.cfg");
         }
         catch { }

         if (sendToDevKit)
         {
            string xboxName = lastXboxTextBox.Text;
            if (xboxName == "")
               return false;

            string xboxPath = lastXboxFolderTextBox.Text;
            if (xboxPath == "")
               return false;
            else
            {
               string c = xboxPath.Substring(xboxPath.Length - 1);
               if (c != @"\")
                  xboxPath += @"\";
            }

            try
            {
               XenonInterface.init();
               if (XenonInterface.mXboxManager != null)
                  XenonInterface.sendFile(xboxName, userCfg, xboxPath + @"startup\user.cfg", true, false);
               XenonInterface.destroy();
            }
            catch { }
         }

         return true;
      }

      //============================================================================
      //============================================================================
      private bool writeXfsTxt(bool sendToDevKit)
      {
         string xfsTxt = Directory.GetCurrentDirectory();
         if (xfsTxt != "")
         {
            string c = xfsTxt.Substring(xfsTxt.Length - 1);
            if (c != @"\")
               xfsTxt += @"\";
         }
         xfsTxt += "xfs.txt";

         try
         {
            if (File.Exists(xfsTxt))
               File.Delete(xfsTxt);
         }
         catch { }
         try
         {
            TextWriter writer = new StreamWriter(xfsTxt);
            writer.WriteLine((xfsCheckBox.Checked ? "xbox " : ";xbox ") + ipDropdown.Text);
            writer.WriteLine("enableArchives");
            writer.WriteLine("disableLooseFiles");
            writer.Close();
         }
         catch { }

         if (sendToDevKit)
         {
            string xboxName = lastXboxTextBox.Text;
            if (xboxName == "")
               return false;

            string xboxPath = lastXboxFolderTextBox.Text;
            if (xboxPath == "")
               return false;
            else
            {
               string c = xboxPath.Substring(xboxPath.Length - 1);
               if (c != @"\")
                  xboxPath += @"\";
            }

            try
            {
               XenonInterface.init();
               if (XenonInterface.mXboxManager != null)
                  XenonInterface.sendFile(xboxName, xfsTxt, xboxPath + @"xfs.txt", true, false);
               XenonInterface.destroy();
            }
            catch { }
         }

         return true;
      }

      //============================================================================
      //============================================================================
      private void writeLocalFiles()
      {
         string path = lastFolderTextBox.Text;
         if (path.Length == 0)
         {
            asyncLog("ERROR: No local folder specified.");
            return;
         }
         else
         {
            string c = path.Substring(path.Length - 1);
            if (c != @"\")
               path += @"\";
         }

         string folderTxt = Directory.GetCurrentDirectory();
         if (folderTxt != "")
         {
            string c = folderTxt.Substring(folderTxt.Length - 1);
            if (c != @"\")
               folderTxt += @"\";
         }
         folderTxt += "folder.txt";

         try
         {
            TextWriter writer = new StreamWriter(folderTxt);
            writer.WriteLine("folder.txt");
            writer.Close();
         }
         catch { }

         try
         {
            Directory.CreateDirectory(path + "savegame");
         }
         catch { }

         try
         {
            Directory.CreateDirectory(path + "recordgame");
         }
         catch { }

         try
         {
            Directory.CreateDirectory(path + "screenshots");
         }
         catch { }

         try 
         {
            File.Copy(folderTxt, path + "savegame\\folder.txt");
            File.Copy(folderTxt, path + "recordgame\\folder.txt");
            File.Copy(folderTxt, path + "screenshots\\folder.txt");
         }
         catch { }

         writeUserCfg(false);
         writeXfsTxt(false);
      }

      //============================================================================
      //============================================================================
      private void deleteLocalBuild()
      {
         int index = historyDropdown.SelectedIndex;
         if (index != -1 && index < mSettings.mGets.Count)
         {
            SettingsGet settingsGet = mSettings.mGets[index];
            string localFolder = settingsGet.mFolder;
            if (localFolder.Length == 0)
            {
               asyncLog("ERROR: No build to delete.");
               return;
            }
            else
            {
               string c = localFolder.Substring(localFolder.Length - 1);
               if (c != @"\")
                  localFolder += @"\";
            }

            disableButtons();
            cancelButton.Enabled = false;

            log("Deleting local files...");
            deleteLocalFiles(localFolder);

            if (settingsGet.mCopies.Count > 0)
            {
               log("Deleting Xbox files...");
               lastCopiedCheckBox.Checked = false;
               lastXboxTextBox.Text = "";
               lastXboxFolderTextBox.Text = "";
               foreach (SettingsCopy settingsCopy in settingsGet.mCopies)
                  deleteXboxFiles(settingsCopy.mXbox, settingsCopy.mFolder, ProcessType.cDelete);
            }

            mSettings.mGets.Remove(settingsGet);

            if (historyDropdown.Items.Count > index)
            {
               historyDropdown.Items.RemoveAt(index);
               if (historyDropdown.Items.Count > 0)
                  historyDropdown.SelectedIndex = 0;
               else
               {
                  historyDropdown.Text = "";
                  lastBuildTextBox.Text = "";
                  lastFileSetTextBox.Text = "";
                  lastFolderTextBox.Text = "";
                  lastDeleteFilesCheckBox.Checked = false;
                  lastSeparateFolderCheckBox.Checked = false;
               }
            }

            log("Completed.");

            enableButtons();
         }
      }

      //============================================================================
      //============================================================================
      private void deleteXboxBuild()
      {
         if (lastXboxTextBox.Text != "" && lastXboxFolderTextBox.Text != "")
         {
            int index = historyDropdown.SelectedIndex;
            if (index != -1 && index < mSettings.mGets.Count)
            {
               disableButtons();
               cancelButton.Enabled = false;

               SettingsGet settingsGet = mSettings.mGets[index];
               foreach (SettingsCopy settingsCopy in settingsGet.mCopies)
               {
                  if (settingsCopy.mXbox == lastXboxTextBox.Text)
                  {
                     settingsGet.mCopies.Remove(settingsCopy);
                     break;
                  }
               }

               log("Deleting Xbox files...");
               deleteXboxFiles(lastXboxTextBox.Text, lastXboxFolderTextBox.Text, ProcessType.cDelete);

               lastCopiedCheckBox.Checked = false;
               lastXboxTextBox.Text = "";
               lastXboxFolderTextBox.Text = "";

               enableButtons();
            }
         }
      }

      //============================================================================
      //============================================================================
      private void rebootXbox()
      {
         if (xboxDropdown.Text != "")
         {
            disableButtons();

            log("Rebooting Xbox...");

            try
            {
               XenonInterface.init();
               if (XenonInterface.mXboxManager != null)
               {
                  XenonInterface.reboot(xboxDropdown.Text, null, null, null, XboxRebootFlags.Cold);
                  log("Completed.");
               }
               XenonInterface.destroy();
            }
            catch
            {
               log("ERROR: Unable to reboot Xbox.");
            }

            enableButtons();

            /*
            log("Rebooting Xbox...");

            string xbrebootPath = @"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\xbreboot.exe";
            bool xbrebootExists = false;
            try
            {
               xbrebootExists = File.Exists(xbrebootPath);
            }
            catch { }
            if (!xbrebootExists)
            {
               log("ERROR: Can't find xbreboot program.");
               return;
            }

            mProcessType = ProcessType.cReboot;
            mProcessCaller = new ProcessCaller(this);
            mProcessCaller.FileName = xbrebootPath;
            mProcessCaller.Arguments = @"/X:" + lastXboxTextBox.Text;
            mProcessCaller.StdErrReceived += new DataReceivedHandler(writeProcessOutput);
            mProcessCaller.StdOutReceived += new DataReceivedHandler(writeProcessOutput);
            mProcessCaller.Completed += new EventHandler(processCompleted);
            mProcessCaller.Cancelled += new EventHandler(processCancelled);
            mProcessCaller.Start();
            //mProcess = new Process();
            */
         }
      }

      //============================================================================
      //============================================================================
      private void updateLastXboxCopyInfo()
      {
         if (mSettings != null)
         {
            int index = historyDropdown.SelectedIndex;
            if (index != -1 && index < mSettings.mGets.Count)
            {
               SettingsGet settingsGet = mSettings.mGets[index];
               lastXboxTextBox.Text = "";
               lastXboxFolderTextBox.Text = "";
               lastCopiedCheckBox.Checked = false;
               if (settingsGet.mCopies.Count > 0)
               {
                  foreach (SettingsCopy settingsCopy in settingsGet.mCopies)
                  {
                     if (settingsCopy.mXbox == xboxDropdown.Text)
                     {
                        lastXboxTextBox.Text = settingsCopy.mXbox;
                        lastXboxFolderTextBox.Text = settingsCopy.mFolder;
                        lastCopiedCheckBox.Checked = settingsCopy.mCopied;
                        break;
                     }
                  }
               }
               if (!cancelButton.Enabled)
               {
                  if (lastXboxTextBox.Text == "")
                  {
                     launchButton.Enabled = false;
                     deleteXboxButton.Enabled = false;
                  }
                  else
                  {
                     launchButton.Enabled = true;
                     deleteXboxButton.Enabled = true;
                  }
               }
            }
         }
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

      private void pickPreBatchFileButton_Click(object sender, EventArgs e)
      {
         pickPreBatchFile();
      }
      
      private void pickPostBatchFileButton_Click(object sender, EventArgs e)
      {
         pickPostBatchFile();
      }

      private void goButton_Click(object sender, EventArgs e)
      {
         doGetBuild(false, false);
      }

      private void cancelButton_Click(object sender, EventArgs e)
      {
         if (mProcessCaller != null)
            mProcessCaller.Cancel();
         else if (mCopyThread != null)
         {
            log("Cancelling...");
            lastCopiedCheckBox.Checked = false;
            lastXboxTextBox.Text = "";
            lastXboxFolderTextBox.Text = "";
            mCopyCancel = true;
         }
         else if (mCopying)
         {
            Server.shutdown();
            mCopying = false;
            enableButtons();
            log("Completed cancelled.");
            //rebootXbox();
         }
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
            {
               buildTextBox.Text = (String)mBuildFiles[buildListView.SelectedIndices[0]];
               if (cancelButton.Enabled == false)
               {
                  goButton.Enabled = true;
                  getCopyLaunchButton.Enabled = true;
               }
            }
            else
            {
               buildTextBox.Text = @"";
               if (cancelButton.Enabled == false)
               {
                  goButton.Enabled = false;
                  getCopyLaunchButton.Enabled = false;
               }
            }
         }
      }

      private void getCopyLaunchButton_Click(object sender, EventArgs e)
      {
         doGetBuild(true, false);
      }

      private void copyButton_Click(object sender, EventArgs e)
      {
         copyBuild(false);
      }

      private void launchButton_Click(object sender, EventArgs e)
      {
         launchGame();
      }

      private void deleteButton_Click(object sender, EventArgs e)
      {
         deleteXboxBuild();
      }

      private void rebootButton_Click(object sender, EventArgs e)
      {
         rebootXbox();
      }

      private void historyDropdown_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (mSettings != null)
         {
            int index = historyDropdown.SelectedIndex;
            if (index != -1 && index < mSettings.mGets.Count)
            {
               SettingsGet settingsGet = mSettings.mGets[index];
               lastBuildTextBox.Text = settingsGet.mBuild;
               lastFileSetTextBox.Text = settingsGet.mFileSet;
               lastFolderTextBox.Text = settingsGet.mFolder;
               lastDeleteFilesCheckBox.Checked = settingsGet.mDeleteFiles;
               lastSeparateFolderCheckBox.Checked = settingsGet.mSeparateFolder;
               updateLastXboxCopyInfo();
            }
         }
      }

      private void xboxDropdown_SelectedIndexChanged(object sender, EventArgs e)
      {
         updateLastXboxCopyInfo();
      }

      private void deleteLocalButton_Click(object sender, EventArgs e)
      {
         deleteLocalBuild();
      }

      private void copyMethodDropdown_SelectedIndexChanged(object sender, EventArgs e)
      {
         /*
         if (copyMethodDropdown.Text == "xfsCopy")
            ipDropdown.Enabled = true;
         else
            ipDropdown.Enabled = false;
         */
      }

      private void exeDropdown_SelectedIndexChanged(object sender, EventArgs e)
      {
         /*
         if (exeDropdown.Text == "xgameP")
            xfsCheckBox.Enabled = true;
         else
            xfsCheckBox.Enabled = false;
         */
      }
  }
}