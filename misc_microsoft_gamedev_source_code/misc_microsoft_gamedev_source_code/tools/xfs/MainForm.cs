using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Collections;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using System.Diagnostics;
using XDevkit;
using System.Runtime.InteropServices;
         
      
namespace xfs
{
   //============================================================================
   // MainForm
   //============================================================================
   public partial class MainForm : Form
   {
      private const int cMaxEvents = -1;
      private const int cUpdateInterval=250;

      private System.Windows.Forms.Timer updateTimer;

      private int mOptionsLock = 0;

      private ArrayList mDeferredLogMessages;
      private ArrayList mDeferredProcessMessages;

      public string mDefaultIP = null;
      
      private readonly object mDeferredDebugConnectionCommandLock = new object();
      private String mDeferredDebugConnectionCommand;

      //============================================================================
      // MainForm.MainForm
      //============================================================================
      public MainForm()
      {
         InitializeComponent();
         
         mDeferredLogMessages = new ArrayList();
         mDeferredProcessMessages = new ArrayList();

         // rg [2/1/06] - This is a bit of a hack - fix the working directory if it ends in "tools\XFS".
         // clm [3.18.06] - changed to use base directory to allow the editor to launch it w/ the proper directory.
         String path = AppDomain.CurrentDomain.BaseDirectory.ToLower();// Directory.GetCurrentDirectory();

         int toolPathIndex = path.LastIndexOf("\\code\\tools\\xfs", path.Length - 1);
         if (toolPathIndex > -1)
         {
            String oldPath = path;
            path = path.Remove(toolPathIndex);
            path = path + "\\work";
            addEvent("Changed work directory from " + oldPath + " to " + path);
            try
            {
               Directory.SetCurrentDirectory(path);
            }
            catch { }
         }
         else
         {
            toolPathIndex = path.LastIndexOf("\\tools\\xfs", path.Length - 1);
            if (toolPathIndex > -1)
            {
               String oldPath = path;
               path = path.Remove(toolPathIndex);
               addEvent("Changed work directory from " + oldPath + " to " + path);
               try
               {
                  Directory.SetCurrentDirectory(path);
               }
               catch { }
            }
            else
            {
               // jce 10/19/2006 -- additional hack for dealing with "xfs" in folder name instead of tools\xfs
               toolPathIndex = path.LastIndexOf("\\xfs", path.Length - 1);
               if (toolPathIndex > -1)
               {
                  String oldPath = path;
                  path = path.Remove(toolPathIndex);
                  addEvent("Changed work directory from " + oldPath + " to " + path);
                  try
                  {
                     Directory.SetCurrentDirectory(path);
                  }
                  catch { }
               }
            }
         }

         string versionFile = path + "\\version.txt";
         if (!File.Exists(versionFile))
         {
            addEvent("Warning: Unable to find file \"" + versionFile + "\". Work directory may be invalid!");
         }

         workDirTextBox.Text = path;
         
         readOptions();
         readPresetCommands();
         readCommandHistory();
      }

      //============================================================================
      // MainForm.initData
      //============================================================================
      public void initData()
      {
         DebugConnectionManager.setTextCallback(TextCallback);
         
         serverIPDropDown.Text = Server.mIP;
         foreach (string ip in Server.mIPs)
         {
            if (!serverIPDropDown.Items.Contains(ip))
               serverIPDropDown.Items.Add(ip);
         }

         serverNameTextBox.Text = Server.mName;
         
         readOptions();

         try
         {
            XenonInterface.init();
            
            if (XenonInterface.mCurrentConsole != null)
            {
               consoleNameDropdown.Text = XenonInterface.mXboxManager.DefaultConsole;
               foreach (string name in XenonInterface.mXboxManager.Consoles)
               {
                  if (!consoleNameDropdown.Items.Contains(name))
                     consoleNameDropdown.Items.Add(name);
               }
            }
         }
         catch { }

         updateTimer = new System.Windows.Forms.Timer();
         updateTimer.Interval = cUpdateInterval;
         updateTimer.Start();
         updateTimer.Tick += new EventHandler(update);
      }

      //============================================================================
      // MainForm.resetOptions
      //============================================================================
      public void resetOptions()
      {
         mOptionsLock++;
         consoleDirTextBox.Text = @"";
         sourceExeDropdown.Text = @"";
         sourceExeDropdown.Items.Clear();
         dvdEmuDirTextBox.Text = @"";
         xboxCheckBox.Checked = true;
         disableServerTimeoutCheckBox.Checked = false;
         Server.mDisableTimeout = false;
         logFileActivityMenuItem.Checked = false;
         disableAutoResetCheckBox.Checked = false;
         monitorFileChangesCheckBox.Checked = FileMonitor.GetEnabled();
         mOptionsLock--;
      }

      //============================================================================
      // CVD [6.6.08] - Read Script Commands
      //============================================================================
      private ArrayList loadScriptedCommands(String filename, ArrayList commandList)
      {
          try
          {
              using (TextReader reader = new StreamReader(filename))
              {
                  if (reader != null)
                  {
                      for (; ; )
                      {
                          string line = reader.ReadLine();
                          if (line == null)
                              break;
                          line.Trim();
                          if (line.Length == 0)
                              continue;
                          commandList.Add(line);
                      }
                  }
              }
          }
          catch
          {
              MessageBox.Show(String.Concat("Error loading scripted commands from ", filename), "Loading Error!", MessageBoxButtons.OK, MessageBoxIcon.Error);
          }     
          return commandList; 
      }
      //============================================================================
      // CVD [6.6.08] - Read and Save PresetCommands
      //============================================================================
      private void readPresetCommands()
      {
        mCmdPresets.Clear();
        
        if(!File.Exists("xfsCmdPresets.txt"))
            return;
            
        try
        {
            using (TextReader reader = new StreamReader("xfsCmdPresets.txt"))
            {
                if (reader != null)
                {
                    for (; ; )
                    {
                        string line = reader.ReadLine();
                        if (line == null)
                            break;
                        line.Trim();
                        if (line.Length == 0)
                            continue;
                        mCmdPresets.Add(line);

                        if (!cmdOptions.Items.Contains(line))
                            cmdOptions.Items.Add(line);
                    }
                }
            }         
        }
        catch (System.Exception e)
        {
        	
        }
        
      }

       public void savePresetHistory()
       {
         
           try
           {
               using (TextWriter writer = new StreamWriter("xfsCmdPresets.txt"))
               {
                   if (writer != null)
                   {
                       foreach (string str in mCmdPresets)
                           writer.WriteLine(str);

                       writer.Close();
                   }
               }
           }
           catch (System.Exception e)
           {
                MessageBox.Show(e.ToString());
           }
       }      
      
      private void readCommandHistory()
      {
         mCmdHistory.Clear();
         mCmdHistoryIndex = 0;

         if (!File.Exists("xfsCmdHistory.txt"))
             return;
             
         try
         {
            using (TextReader reader = new StreamReader("xfsCmdHistory.txt"))
            {
               if (reader != null)
               {
                  for ( ; ; )
                  {
                     string line = reader.ReadLine();
                     if (line == null) 
                        break;
                     line.Trim();
                     if (line.Length == 0)
                        continue;
                     mCmdHistory.Add(line);

                     if (!cmdOptions.Items.Contains(line))
                        cmdOptions.Items.Add(line);                       
                  }
               }
            }               
            
            mCmdHistoryIndex = mCmdHistory.Count;
         }
         catch (System.Exception e)
         {
         }
      }
      
      public void saveCommandHistory()
      {
         try
         {
            using (TextWriter writer = new StreamWriter("xfsCmdHistory.txt"))
            {
               if (writer != null)
               {
                  foreach (string str in mCmdHistory)
                     writer.WriteLine(str);
                  
                  writer.Close();
               }
            }               
         }
         catch (System.Exception e)
         {
         }      
      }

      //============================================================================
      // MainForm.readOptions
      //============================================================================
      public void readOptions()
      {
         resetOptions();

         mOptionsLock++;

         // Read in options from the xfs.txt file
         try
         {
            TextReader reader = new StreamReader("xfs.txt");
            Regex re = new Regex(@" ");
            if (reader != null)
            {
               try
               {
                  bool enableForXbox = false;
                  bool monitorFileChanges = FileMonitor.GetEnabled();
                  bool firstSourceExe = true;
                  for (; ; )
                  {
                     string line = reader.ReadLine();
                     if (line==null)   break;
                     string[] tokens = re.Split(line);
                     bool nextTokenAsXboxDir = false;
                     bool nextTokenAsSourceExe = false;
                     bool nextTokenAsDefaultIP = false;
                     bool nextTokenAsDvdEmuDir = false;
                     foreach (string token in tokens)
                     {
                        if (token.Length == 0)
                           continue;
                        if (token.Substring(0, 1) == ";")
                           break;
                        if (nextTokenAsXboxDir)
                        {
                           consoleDirTextBox.Text = token;
                           break;
                        }
                        else if (nextTokenAsDvdEmuDir)
                        {
                           dvdEmuDirTextBox.Text = token;
                           break;
                        }
                        else if (nextTokenAsSourceExe)
                        {
                           if (firstSourceExe)
                           {
                              if (token != @"none")
                              {
                                 sourceExeDropdown.Text = token;
                                 sourceExeDropdown.Items.Add(token);
                              }
                              firstSourceExe = false;
                           }
                           else
                              sourceExeDropdown.Items.Add(token);
                           break;
                        }
                        else if (nextTokenAsDefaultIP)
                        {
                           if (mDefaultIP == null)
                              mDefaultIP = token;
                           break;
                        }
                        token.ToLower();
                        if (token.CompareTo(@"xbox") == 0)
                        {
                           enableForXbox = true;
                           break;
                        }
                        else if (token.CompareTo(@"win32") == 0)
                        {
                           break;
                        }
                        else if (token.CompareTo(@"disableServerTimeout") == 0)
                        {
                           disableServerTimeoutCheckBox.Checked = true;
                           Server.mDisableTimeout = true;
                           break;
                        }
                        else if (token.CompareTo(@"logfileactivity") == 0)
                        {
                           logFileActivityMenuItem.Checked = true;
                           break;
                        }
                        else if (token.CompareTo(@"noAutoReset") == 0)
                        {
                           disableAutoResetCheckBox.Checked = true;
                           break;
                        }
                        else if (token.CompareTo(@"xboxdir") == 0)
                        {
                           nextTokenAsXboxDir = true;
                        }
                        else if (token.CompareTo(@"dvdemudir") == 0)
                        {
                           nextTokenAsDvdEmuDir = true;
                        }
                        else if (token.CompareTo(@"sourceexe") == 0)
                        {
                           nextTokenAsSourceExe = true;
                        }
                        else if (token.CompareTo(@"defaultip") == 0)
                        {
                           nextTokenAsDefaultIP = true;
                        }
                        else if (token.CompareTo(@"noMonitorFileChanges") == 0)
                        {
                           monitorFileChanges = false;
                           break;
                        }
                        else if (token.CompareTo(@"disableAutoReset") == 0)
                        {
                           disableAutoResetCheckBox.Checked = true;
                           break;
                        }
                        else if (token.CompareTo(@"enableArchives") == 0)
                        {
                           enableArchivesCheckBox.Checked = true;
                           break;
                        }
                        else if (token.CompareTo(@"disableLooseFiles") == 0)
                        {
                           disableLooseFilesCheckBox.Checked = true;
                           break;
                        }
                        else if (token.CompareTo(@"disableNoDelay") == 0)
                        {
                           Server.mEnableNoDelay = false;
                           break;
                        }
                        else if (token.CompareTo(@"finalBuild") == 0)
                        {
                           finalBuildCheckBox.Checked = true;
                           break;
                        }
                     }
                  }

                  xboxCheckBox.Checked = enableForXbox;

                  monitorFileChangesCheckBox.Checked = monitorFileChanges;
                  if (monitorFileChanges != FileMonitor.GetEnabled())
                     FileMonitor.SetEnabled(monitorFileChanges);
               }
               catch { }
               reader.Close();
            }
            else
            {
               addEvent("Error: Couldn't load and parse xfs.txt");
            }
         }
         catch { addEvent("Error: Couldn't load and parse xfs.txt"); }

         mOptionsLock--;
      }

      //============================================================================
      // MainForm.writeOptions
      //============================================================================
      public void writeOptions(bool copyToXbox)
      {
         if (mOptionsLock > 0)
            return;

         try
         {
            string path=@"xfs.txt";
            try
            {
               if (File.Exists(path))
               {
                  File.Delete(path);
               }
            }
            catch { }

            TextWriter writer = new StreamWriter(path);

            if (!xboxCheckBox.Checked)
               writer.Write(";");
            writer.WriteLine("xbox " + Server.mIP);

            //if (!windowsCheckBox.Checked)
            //   writer.Write(";");
            //writer.WriteLine("win32 " + Server.mIP);

            if (!disableServerTimeoutCheckBox.Checked)
               writer.Write(";");
            writer.WriteLine("disableServerTimeout");

            if (!logFileActivityMenuItem.Checked)
               writer.Write(";");
            writer.WriteLine("logfileactivity");

            if (!disableAutoResetCheckBox.Checked)
               writer.Write(";");
            writer.WriteLine("noAutoReset");

            if (consoleDirTextBox.Text.Length > 0)
               writer.WriteLine("xboxdir " + consoleDirTextBox.Text);
            else
               writer.WriteLine(";xboxdir none");

            if (dvdEmuDirTextBox.Text.Length > 0)
               writer.WriteLine("dvdemudir " + dvdEmuDirTextBox.Text);
            else
               writer.WriteLine(";dvdemudir none");

            if (sourceExeDropdown.Text.Length > 0)
               writer.WriteLine("sourceexe " + sourceExeDropdown.Text);
            else
               writer.WriteLine("sourceexe none");

            if (monitorFileChangesCheckBox.Checked)
               writer.Write(";");
            writer.WriteLine("noMonitorFileChanges");

            if (!enableArchivesCheckBox.Checked)
               writer.Write(";");
            writer.WriteLine("enableArchives");

            if (!disableLooseFilesCheckBox.Checked)
               writer.Write(";");
            writer.WriteLine("disableLooseFiles");

            writer.WriteLine("defaultip " + Server.mIP);
            int exeCount = 0;
            foreach (string exe in sourceExeDropdown.Items)
            {
               if (exe != sourceExeDropdown.Text)
               {
                  writer.WriteLine("sourceexe " + exe);
                  exeCount++;
                  if (exeCount == 5)
                     break;
               }
            }

            if (Server.mEnableNoDelay)
               writer.Write(";");
            writer.WriteLine("disableNoDelay");

            if (!finalBuildCheckBox.Checked)
               writer.Write(";");
            writer.WriteLine("finalBuild");

            writer.Close();

            // Copy to the dvd emu directory
            string dvdEmuDir = dvdEmuDirTextBox.Text;
            if (dvdEmuDir.Length > 0)
            {
               try
               {
                  File.Copy(path, dvdEmuDir + "\\" + path, true);
                  addEvent("Successfully copied file xfs.txt to " + dvdEmuDir);
               }
               catch
               {
                  addEvent("Error: Couldn't copy xfs.txt to " + dvdEmuDir);
               }
            }

            // Copy to the xbox devkit
            if (copyToXbox)
            {
               string xboxDir = consoleDirTextBox.Text;
               if (xboxDir.Length > 0 && XenonInterface.mXboxManager != null)
               {
                  try
                  {
                     XenonInterface.openConsole(consoleNameDropdown.Text);
                     string dest = xboxDir + "\\xfs.txt";
                     if (XenonInterface.eErrorType.cOK == XenonInterface.sendFile("xfs.txt", dest, true, false))
                        addEvent("Successfully sent file xfs.txt to " + dest);
                     else
                        addEvent("Error: Couldn't send xfs.txt to Xbox");
                  }
                  catch
                  {
                     addEvent("Error: Couldn't send xfs.txt to Xbox");
                  }

               }
            }
         }
         catch { }
      }
      
      static private int mUpdateCounter;

      //============================================================================
      // MainForm.update
      //============================================================================
      public void update(object sender, EventArgs eArgs)
      {
         if (sender == updateTimer)
         {
            mUpdateCounter++;

            try
            {
               ArrayList events = Server.getEventList();
               if (events.Count > 0)
               {
                  bool doLog = logFileActivityMenuItem.Checked;
                  bool end = (eventListBox.SelectedIndex == eventListBox.Items.Count - 1);
                  int skipCount = 0;
                  if (doLog && cMaxEvents != -1)
                  {
                     int newCount = events.Count;
                     int listCount = eventListBox.Items.Count;
                     if (listCount + newCount > cMaxEvents)
                     {
                        if (newCount >= cMaxEvents)
                        {
                           skipCount = newCount - cMaxEvents;
                           eventListBox.Items.Clear();
                        }
                        else
                        {
                           int delCount = (listCount + newCount) - cMaxEvents;
                           for (int i = 0; i < delCount; i++)
                           {
                              eventListBox.Items.RemoveAt(0);
                           }
                        }
                     }
                  }
                  try
                  {
                     foreach (NetEvent netEvent in events)
                     {
                        if (doLog)
                        {
                           if (skipCount > 0)
                              skipCount--;
                           else
                              addEvent(netEvent.ToString());
                        }
                        switch (netEvent.mType)
                        {
                           case NetEvent.EventType.ClientConnect:
                              clientListBox.Items.Add(netEvent.mText);
                              clientListBox.SetSelected(clientListBox.Items.Count - 1, true);

                              FileMonitor.init(workDirTextBox.Text);

                              clearToolStripMenuItem_Click(this, null);
                              
                              XenonInterface.resetRunningShellCheck();

                              break;

                           case NetEvent.EventType.ClientDisconnect:
                              clientListBox.Items.Remove(netEvent.mText);

                              FileMonitor.destroy();

                              // rg [7/19/06] - This asynchronous clear was causing events to be missed, XFS seems more stable if this is remarked out.
                              //Server.forceClearWaitingEvents();
                              break;

                           case NetEvent.EventType.ConnectionOpen:

                              break;

                           case NetEvent.EventType.ConnectionClose:
                              break;

                           case NetEvent.EventType.ConnectionReset:
                              clearToolStripMenuItem_Click(this, null);
                              
                              // rg [7/19/06] - This asynchronous clear was causing events to be missed, XFS seems more stable if this is remarked out.
                              //Server.forceClearWaitingEvents();
                              break;

                           case NetEvent.EventType.ResetClient:
                              //XenonInterface.reset();
                              break;

                           case NetEvent.EventType.FileOpen:
                              FileMonitor.addFile(netEvent.mText);

                              openFileListBox.Items.Add(netEvent.mText);
                              if (doLog)
                              {
                                 string fileName = netEvent.mText.ToLower();
                                 if (!fileListBox.Items.Contains(fileName))
                                 {
                                    fileListBox.Items.Add(fileName);
                                    fileListBox.SelectedIndex = dirListBox.Items.Count - 1;
                                    fileListBox.SelectedIndex = -1;
                                 }

                                 int index = fileName.LastIndexOf('\\');
                                 if (index > 0)
                                 {
                                    string dir = fileName.Substring(0, index);
                                    if (!dirListBox.Items.Contains(dir))
                                    {
                                       dirListBox.Items.Add(dir);
                                       dirListBox.SelectedIndex = dirListBox.Items.Count - 1;
                                       dirListBox.SelectedIndex = -1;
                                    }
                                 }
                              }
                              break;

                           case NetEvent.EventType.FileClose:
                              openFileListBox.Items.Remove(netEvent.mText);
                              break;

                           case NetEvent.EventType.Exception:
                              logException(netEvent.mText);
                              break;
                        }
                     }
                  }
                  catch (Exception ex)
                  {
                     logException(ex.ToString());
                  }
                  if (doLog && end)
                  {
                     int count = eventListBox.Items.Count;
                     if (count > 0)
                     {
                        eventListBox.SelectedIndex = count - 1;
                        eventListBox.SelectedIndex = -1;
                     }
                  }
                  events.Clear();
               }

               updateLogMessages();

               updateProcessMessages();
               
               checkForShell();
               
               updateButtons();
               
               updateDeferredDebugConnectionCommand();
            }
            catch (Exception e)
            {
               logException(e.ToString());
               if (System.Diagnostics.Debugger.IsAttached)
                  throw e;
            }
         }
      }

      //============================================================================
      // 
      //============================================================================
      private void logException(string e)
      {
         if (enableExceptionLoggingMenuItem.Checked)
         {
            try
            {
               FileStream fs = File.Open("xfsExceptions.txt", FileMode.OpenOrCreate);
               StreamWriter sw = new StreamWriter(fs);
               sw.WriteLine(e);
               sw.Flush();
               sw.Close();
            }
            catch (System.Exception)
            {
            }
         }
      }

      //============================================================================
      // updateDeferredDebugConnectionCommand
      //============================================================================
      private void updateDeferredDebugConnectionCommand()
      {
         if (mDeferredDebugConnectionCommand == null)
            return;
            
         if (DebugConnectionManager.hasActiveConnections())
         {
            lock (mDeferredDebugConnectionCommandLock)
            {
               if (mDeferredDebugConnectionCommand.Length > 0)
               {
                  DebugConnectionManager.sendCommand(mDeferredDebugConnectionCommand);
                  
                  mDeferredDebugConnectionCommand = "";
               }
            }
         }
      }
      
      //============================================================================
      // updateButtons
      //============================================================================
      private void updateButtons()
      {      
         if (DebugConnectionManager.hasActiveConnections())
         {
            screenshotButton.Enabled = true;
            QuickViewButton.Enabled = true;
            DisableUIButton.Enabled = true;
         }
         else
         {
            screenshotButton.Enabled = false;                
            QuickViewButton.Enabled = false;
            DisableUIButton.Enabled = false;
         }
      }               
      
      //============================================================================
      // checkForShell
      //============================================================================
      private void checkForShell()
      {
         if (!disableAutoResetCheckBox.Checked)
         {
            if (XenonInterface.getRunningShell())
            {
               if (Server.hasActiveConnections())
               {
                  addEvent("The console is running the launcher, but there are active file connections. Attempting to reset the server.");
                  if (!Server.reset(true))
                     addEvent("Server reset timed out!");
                  else
                     addEvent("Server successfully reset.");  
               }
               if (DebugConnectionManager.hasActiveConnections())   
               {
                  addEvent("The console is running the launcher, but there are active debug channel connections. Resetting the debug connection manager.");
                  DebugConnectionManager.reset();
               }
            }               
         }
      }

      //============================================================================
      // processExecMessage
      //============================================================================
      private void processExecMessage(string str)
      {  
         try
         {
            int k = str.IndexOf(@">");
            if (k != -1)
               str = str.Substring(k + 1);

            str = str.Trim();
               
            if (str.Length == 0)
               return;
                     
            if (str[str.Length - 1] == '>')
            {
               k = str.LastIndexOf('<');
               if (k != -1)
                  str = str.Substring(0, k);
            }
                     
            int sepIndex = str.IndexOf("\"");
            if (sepIndex == -1)
            {
               sepIndex = str.IndexOf(" ");
            }
            else if (sepIndex != str.Length - 1)
            {
               int firstQuoteIndex = sepIndex;
               sepIndex = str.IndexOf("\"", sepIndex + 1);
               if (sepIndex != -1)
               {
                  str = str.Substring(firstQuoteIndex + 1);
                  sepIndex -= (firstQuoteIndex + 1);
               }
            }
            
            string execName, args = "";
                     
            if (sepIndex == -1)
               execName = str;
            else
            {
               execName = str.Substring(0, sepIndex).Trim();
               if (sepIndex < str.Length - 1)
                  args = str.Substring(sepIndex + 1).Trim();
            }
                     
            ProcessStartInfo startInfo = new ProcessStartInfo(execName);

            //startInfo.WindowStyle = ProcessWindowStyle.Minimized;
            startInfo.Arguments = args;
            startInfo.ErrorDialog = true;

            Process.Start(startInfo);
         }
         catch
         {
         }            
      }
      
      //============================================================================
      // updateLogMessages
      //============================================================================
      private void updateLogMessages()
      {
         lock (mDeferredLogMessages)
         {
            if (mDeferredLogMessages.Count > 0)
            {
               foreach (string str in mDeferredLogMessages)
               {
                  if (str.StartsWith("<cmd>"))
                     addConsoleCommand(str);
                  else if (str.StartsWith("<exec>"))
                     processExecMessage(str);
                  else
                     logMonitor1.submitMessage(str, -1);
               }
               mDeferredLogMessages.Clear();
            }
         }
      }
      //============================================================================
      // updateProcessMessages
      //============================================================================
      private void updateProcessMessages()
      {
         lock (mDeferredProcessMessages.SyncRoot)
         {
            if (mDeferredProcessMessages.Count > 0)
            {
               foreach (string str in mDeferredProcessMessages)
               {
                  eventListBox.Items.Add("IPC:" + str);
                  
                  if (str.Contains("launchGame"))
                  {
                     try
                     {
                        XBOX_PROCESS_INFO processInfo = XenonInterface.mCurrentConsole.RunningProcessInfo;
                        if (processInfo.ProgramName.Contains("xshell.xex"))
                        {
                           DebugConnectionManager.reset();
                           launchGame();
                        }
                     }
                     catch (Exception)
                     {
                        //                  Debug.WriteLine("XenonInterface get RunningProcessInfo exception: " + e.ToString());
                        DebugConnectionManager.reset();
                        launchGame();
                     }
                     
                  }
                  else 
                  {
                     string msg = "";

                     if (str[0] == '!')//just pass this message directly to the 360..
                     {
                        msg = str.Substring(1) ;//remove the "!"
                     }
                     else
                     {
                        char[] delim = new char[] { ' ', ',' };
                        string[] tokens = str.Split(delim);
                        
                        int count = 0;
                        foreach(string tok in tokens)
                        {
                           if(count == 0)
                              msg = tok + "(";
                           else
                              msg += tok;

                           count++;
                        }
                        msg += ")";
                     }

                     int msgLen = msg.Length;
                     //DebugConnectionManager.sendCommand(msg);
                     lock (mDeferredDebugConnectionCommandLock)
                     {
                        if (mDeferredDebugConnectionCommand != null)
                        {
                           mDeferredDebugConnectionCommand = msg;
                        }
                        else
                        {
                           mDeferredDebugConnectionCommand = msg;
                        }
                     }
                  }
                  
               }
               mDeferredProcessMessages.Clear();
            }
         }
      }
      //============================================================================
      // addProcessCommand
      //============================================================================
      public void addInterprocessCommand(string txt)
      {
         lock (mDeferredProcessMessages.SyncRoot)
         {
            mDeferredProcessMessages.Add(txt);
         }
      }
      //============================================================================
      // addCommandConsoleText
      //============================================================================
      public void addCommandConsoleText(string msg)
      {
         cmdOutput.Items.Add(msg);
                  
         if (cmdOutput.Items.Count > 0)
         {
            cmdOutput.SelectedIndex = cmdOutput.Items.Count - 1;
            cmdOutput.SelectedIndex = -1;
         }
      }

      //============================================================================
      // MainForm.pickWorkDirectory
      //============================================================================
      private void pickWorkDirectory()
      {
         FolderBrowserDialog dialog = new System.Windows.Forms.FolderBrowserDialog();
         dialog.Description = "Select the working directory.";
         dialog.ShowNewFolderButton = false;
         dialog.RootFolder = Environment.SpecialFolder.MyComputer;
         dialog.SelectedPath = Directory.GetCurrentDirectory();
         DialogResult result = dialog.ShowDialog();
         if (result == DialogResult.OK)
         {
            Directory.SetCurrentDirectory(dialog.SelectedPath);
            workDirTextBox.Text = Directory.GetCurrentDirectory();

            string versionFile = workDirTextBox.Text + "\\version.txt";
            if (!File.Exists(versionFile))
            {
               addEvent("Warning: Unable to find file \"" + versionFile + "\". Work directory may be invalid!");
            }

            readOptions();
         }
      }

      //============================================================================
      // MainForm.pickDvdEmuDirectory
      //============================================================================
      private void pickDvdEmuDirectory()
      {
         String saveWorkDir = Directory.GetCurrentDirectory();
         FolderBrowserDialog dialog = new System.Windows.Forms.FolderBrowserDialog();
         dialog.Description = "Select the DVD emulation directory.";
         dialog.ShowNewFolderButton = false;
         dialog.RootFolder = Environment.SpecialFolder.MyComputer;
         if (dvdEmuDirTextBox.Text.Length == 0)
            dialog.SelectedPath = Directory.GetCurrentDirectory();
         else
            dialog.SelectedPath = dvdEmuDirTextBox.Text;
         DialogResult result = dialog.ShowDialog();
         Directory.SetCurrentDirectory(saveWorkDir);
         if (result == DialogResult.OK)
         {
            dvdEmuDirTextBox.Text = dialog.SelectedPath;
            writeOptions(false);
         }
      }

      //============================================================================
      // MainForm.addEvent
      //============================================================================
      private void addEvent(string eventText)
      {
         eventListBox.Items.Add(eventText);
         int count = eventListBox.Items.Count;
         if (count > 0)
            eventListBox.SelectedIndex = count - 1;
      }

      //============================================================================
      // MainForm.setWaiting
      //============================================================================
      private void setWaiting()
      {
         Cursor = Cursors.WaitCursor;
      }

      //============================================================================
      // MainForm.resetWaiting
      //============================================================================
      private void resetWaiting()
      {
         this.Cursor = Cursors.Arrow;
      }

      //============================================================================
      // MainForm.addSourceExe
      //============================================================================
      void addSourceExe(string exe)
      {
         string exeLower = exe.ToLower();
         foreach (string item in sourceExeDropdown.Items)
         {
            string itemLower = item.ToLower();
            if (itemLower == exeLower)
               return;
         }
         sourceExeDropdown.Items.Insert(0, exe);
      }

      //============================================================================
      // MainForm.launchGame
      //============================================================================
      private void launchGame()
      {
         setWaiting();

//         cmdOutput.Items.Clear();
         logMonitor1.clearAllLogs();
         fileListBox.Items.Clear();
         dirListBox.Items.Clear();

         writeOptions(true);
         
         try
         {
            // rg [6/5/06] - Should do a time stamp check here. But, the copy is so fast that it's probably not needed.
            if (copyToConsole())
            {
               if (sourceExeDropdown.Text.Length > 0)
               {
                  string consoleExe;
                  consoleExe = Path.GetFileName(sourceExeDropdown.Text);
                  if (consoleDirTextBox.Text.Length > 0 && consoleExe.Length > 0 && XenonInterface.mXboxManager != null)
                  {
                     string gameExe = consoleDirTextBox.Text + "\\" + consoleExe;
                     XenonInterface.reboot(consoleNameDropdown.Text, gameExe, consoleDirTextBox.Text, "", XboxRebootFlags.Title);
                     addSourceExe(sourceExeDropdown.Text);
                     addEvent("Successfully started Xbox executable " + gameExe);
                  }
               }
            }
         }
         catch
         {
            addEvent("Unable to start Xbox game");
         }
         resetWaiting();
      }

      //============================================================================
      // MainForm.copyCinematics
      //============================================================================
      private void copyCinematics()
      {
         setWaiting();

         try
         {
            FileInfo[] fileInfoList = null;
            DirectoryInfo dirInfo = new DirectoryInfo("video\\");
            fileInfoList = dirInfo.GetFiles("*.bik");
            if (fileInfoList != null)
            {
               bool failed = false;
               foreach (FileInfo fileInfo in fileInfoList)
               {
                  addEvent(fileInfo.Name+"...");
                  string source = workDirTextBox.Text + "\\video\\" + fileInfo.Name;
                  string dest = consoleDirTextBox.Text + "\\video\\" + fileInfo.Name;
                  if (XenonInterface.eErrorType.cOK != XenonInterface.sendFile(consoleNameDropdown.Text, source, dest, true, true))
                  {
                     addEvent("Cinematics copy failed");
                     failed = true;
                     break;
                  }
               }
               if (!failed)
                  addEvent("Cinematics copy complete");
            }
            else
               addEvent("No cinematics to copy");
         }
         catch
         {
            addEvent("Unable to copy cinematics");
         }

         try
         {
            FileInfo[] fileInfoList = null;
            DirectoryInfo dirInfo = new DirectoryInfo("video\\talkingheads\\");
            fileInfoList = dirInfo.GetFiles("*.bik");
            if (fileInfoList != null)
            {
               bool failed = false;
               foreach (FileInfo fileInfo in fileInfoList)
               {
                  addEvent(fileInfo.Name + "...");
                  string source = workDirTextBox.Text + "\\video\\talkingheads\\" + fileInfo.Name;
                  string dest = consoleDirTextBox.Text + "\\video\\talkingheads\\" + fileInfo.Name;
                  if (XenonInterface.eErrorType.cOK != XenonInterface.sendFile(consoleNameDropdown.Text, source, dest, true, true))
                  {
                     addEvent("Talking heads copy failed");
                     failed = true;
                     break;
                  }
               }
               if (!failed)
                  addEvent("Talking heads copy complete");
            }
            else
               addEvent("No talking heads to copy");
         }
         catch
         {
            addEvent("Unable to copy talking heads");
         }


         resetWaiting();
      }

      //============================================================================
      // MainForm.rebootConsole
      //============================================================================
      private void rebootConsole()
      {
         setWaiting();
         try
         {
            if (XenonInterface.mXboxManager != null)
            {
               XenonInterface.reboot(consoleNameDropdown.Text, null, null, null, XboxRebootFlags.Cold);
               addEvent("Successfully rebooted Xbox");
            }
         }
         catch
         {
            addEvent("Unable to reboot Xbox");
         }
         resetWaiting();
      }

      //============================================================================
      // MainForm.copyToConsole
      //============================================================================
      private bool copyToConsole()
      {
         bool status = false;

         setWaiting();
         try
         {
            if (sourceExeDropdown.Text.Length > 0)
            {
               string consoleExe;
               consoleExe = Path.GetFileName(sourceExeDropdown.Text);
               if (consoleDirTextBox.Text.Length > 0 && consoleExe.Length > 0 && sourceExeDropdown.Text.Length > 0)
               {
                  string source = sourceExeDropdown.Text;
                  string dest = consoleDirTextBox.Text + "\\" + consoleExe;

                  if (XenonInterface.eErrorType.cOK == XenonInterface.sendFile(consoleNameDropdown.Text, source, dest, true, false))
                  {
                     addEvent("Successfully copied file from " + source + " to " + dest);
                     addSourceExe(sourceExeDropdown.Text);
                     status = true;
                  }
                  else
                  {
                     addEvent("Failed copying file from " + source + " to " + dest);
                  }
               }
            }
         }
         catch
         {
            addEvent("Unable to copy file(s) to Xbox");
         }
         resetWaiting();

         return status;
      }

      //============================================================================
      // MainForm.captureScreenshot
      //============================================================================
      private void captureScreenshot()
      {
         setWaiting();
         try
         {
            if (XenonInterface.mXboxManager != null)
            {
               string fileName = Environment.GetFolderPath(Environment.SpecialFolder.Personal) + "\\xbox360.bmp";
               XenonInterface.captureScreenShot(fileName);
               Process.Start(fileName);
               addEvent("Successfully captured screenshot to file " + fileName);
            }
         }
         catch
         {
            addEvent("Unable to capture screenshot!");
         }
         resetWaiting();
      }

      //============================================================================
      // MainForm.resetServer
      //============================================================================
      private void resetServer()
      {
         setWaiting();
         Server.reset(false);
         DebugConnectionManager.reset();
         eventListBox.Items.Clear();
         fileListBox.Items.Clear();
         dirListBox.Items.Clear();
//         cmdOutput.Items.Clear();
         logMonitor1.clearAllLogs();
         clientListBox.Items.Clear();
         openFileListBox.Items.Clear();
         addEvent("Server reset");
         resetWaiting();
      }

      //============================================================================
      // MainForm.saveLog
      //============================================================================
      private void saveLog()
      {
         ListBox listBox = null;
         string path = null;

         if(logTabControl.SelectedTab==logPage)
         {
            listBox = eventListBox;
            path = @"xfs_log.txt";
         }
         else if(logTabControl.SelectedTab==filePage)
         {
            listBox = fileListBox;
            path = @"xfs_files.txt";
         }
         else if(logTabControl.SelectedTab==dirPage)
         {
            listBox = dirListBox;
            path = @"xfs_dirs.txt";
         }
         else if(logTabControl.SelectedTab==phxOutputPage)
         {
            logMonitor1.saveLog();
            return;
         }
         else if(logTabControl.SelectedTab==cmdLinePage)
         {
            listBox = cmdOutput;
            path = @"xfs_cmdLine.txt";
         }
         else
            return;

         try
         {
            if (File.Exists(path))
               File.Delete(path);
         }
         catch { }
         try
         {
            StreamWriter writer = new StreamWriter(path);
            foreach (string name in listBox.Items)
               writer.WriteLine(name);
            writer.Close();
         }
         catch { }

         Directory.SetCurrentDirectory(workDirTextBox.Text);
      }

      //============================================================================
      // MainForm.clearLog
      //============================================================================
      private void clearLog()
      {
         if (logTabControl.SelectedTab == logPage)
            eventListBox.Items.Clear();
         else if (logTabControl.SelectedTab == filePage)
            fileListBox.Items.Clear();
         else if (logTabControl.SelectedTab == dirPage)
            dirListBox.Items.Clear();
         else if (logTabControl.SelectedTab == phxOutputPage)
            logMonitor1.clearActiveLog();
         else if (logTabControl.SelectedTab == cmdLinePage)
            cmdOutput.Items.Clear();
         
      }

/*
      //============================================================================
      // MainForm.XenonTEXTCallback
      //============================================================================
      private void XenonTEXTCallback(string Source, string Notification)
      {
         if (Notification == null)
            return;

         // This is called from a worker thread
         lock (mDeferredLogMessages) 
         {
            mDeferredLogMessages.Add(Notification);
         }
      }
*/

      //============================================================================
      // addConsoleCommand
      //============================================================================
      private void addConsoleCommand(string cmd)
      {
         cmd = cmd.Substring(5);

//         int i = cmd.IndexOf('(');
//         if (i >= 0)
//            cmd = cmd.Substring(0, i);

         if (!cmdOptions.Items.Contains(cmd))
         {
            
            cmdOptions.Items.Add(cmd);
         }
         
      }

      //============================================================================
      // MainForm.TextCallback
      //============================================================================
      private void TextCallback(String text)
      {
         lock (mDeferredLogMessages)
         {
            mDeferredLogMessages.Add(text);
         }
      }

      private void setDirectoryToolStripMenuItem_Click(object sender, EventArgs e)
      {
         pickWorkDirectory();
      }

      private void exitToolStripMenuItem_Click(object sender, EventArgs e)
      {
         Close();
      }

      private void chooseDirectory_Click(object sender, EventArgs e)
      {
         pickWorkDirectory();
      }

      private void clearToolStripMenuItem_Click(object sender, EventArgs e)
      {
         //eventListBox.Items.Clear();
         fileListBox.Items.Clear();
         dirListBox.Items.Clear();
         //cmdOutput.Items.Clear();
         logMonitor1.clearAllLogs();
      }

      private void xboxCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         writeOptions(true);
      }

      private void logFileActivityMenuItem_Click(object sender, EventArgs e)
      {
         writeOptions(false);
      }

      private void applyConsoleDir_Click(object sender, EventArgs e)
      {
         writeOptions(true);
      }

      private void rebootToolStripMenuItem_Click(object sender, EventArgs e)
      {
         rebootConsole();
      }

      private void exploreToolStripMenuItem_Click(object sender, EventArgs e)
      {
         setWaiting();
         try
         {
            if (XenonInterface.mXboxManager != null)
            {
               string xboxDir = consoleDirTextBox.Text;
               XenonInterface.mXboxManager.OpenWindowsExplorer(consoleNameDropdown.Text, null);
            }
         }
         catch { }
         resetWaiting();
      }

      private void screenCaptureToolStripMenuItem_Click(object sender, EventArgs e)
      {
         captureScreenshot();
      }

      private void launchGameToolStripMenuItem_Click(object sender, EventArgs e)
      {
         launchGame();
      }

      private void startGame_Click(object sender, EventArgs e)
      {
         launchGame();
      }

      private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
      {
         writeOptions(false);
      }

      private void exploreToolStripMenuItem1_Click(object sender, EventArgs e)
      {
         Process.Start(Directory.GetCurrentDirectory());
      }

      private void MainForm_Resize(object sender, EventArgs e)
      {
         if (FormWindowState.Minimized == WindowState)
         {
            Hide();
         }
      }

      private void notifyIcon1_DoubleClick(object sender, EventArgs e)
      {
         Show();
         WindowState = FormWindowState.Normal;
         Focus();
      }

      private void toolStripMenuItem1_Click(object sender, EventArgs e)
      {
         Show();
         WindowState = FormWindowState.Normal;
      }

      private void exitToolStripMenuItem1_Click(object sender, EventArgs e)
      {
         Close();
      }

      private void chooseExe_Click(object sender, EventArgs e)
      {
         String saveWorkDir = Directory.GetCurrentDirectory();
         OpenFileDialog dialog = new OpenFileDialog();
         try
         {
            if(sourceExeDropdown.Text.Length>0)
               dialog.InitialDirectory = Path.GetDirectoryName(sourceExeDropdown.Text);
         }
         catch { }
         if(dialog.InitialDirectory.Length==0)
            dialog.InitialDirectory = Directory.GetCurrentDirectory();
         dialog.Filter = "Executable files (*.xex)|*.xex|All files (*.*)|*.*";
         dialog.FilterIndex = 1;
         dialog.RestoreDirectory = true;
         if (dialog.ShowDialog() == DialogResult.OK)
         {
            FileStream myStream = (FileStream)dialog.OpenFile();
            if (myStream != null)
            {
               sourceExeDropdown.Text = myStream.Name;
               addSourceExe(sourceExeDropdown.Text);
               myStream.Close();
            }
         }
         Directory.SetCurrentDirectory(saveWorkDir);
      }

      private void copyExe_Click(object sender, EventArgs e)
      {
         copyToConsole();
      }

      private void makeDefault_Click(object sender, EventArgs e)
      {
         setWaiting();

         try
         {
            if (XenonInterface.mXboxManager != null)
            {
               string name = consoleNameDropdown.Text;
               XenonInterface.openConsole(name);
               addEvent("Set default console to " + name);
            }
         }
         catch
         {
            addEvent("Unable to set default console");
         }
         
         resetWaiting();
      }

      private void button2_Click(object sender, EventArgs e)
      {
         resetServer();
      }

      private void button3_Click(object sender, EventArgs e)
      {
         rebootConsole();
      }

      private void resetServerMenuItem_Click(object sender, EventArgs e)
      {
         resetServer();
      }

      private void resetToolStripMenuItem_Click(object sender, EventArgs e)
      {
         resetServer();
      }

      private void reboot360ToolStripMenuItem_Click(object sender, EventArgs e)
      {
         rebootConsole();
      }

      private void saveLogButton_Click(object sender, EventArgs e)
      {
         saveLog();
      }

      private void clearLogButton_Click(object sender, EventArgs e)
      {
         clearLog();
      }
      
      private ArrayList mCmdHistory = new ArrayList();
      private int mCmdHistoryIndex;
      
      private ArrayList mCmdPresets = new ArrayList();

      private void cmdSubmit_Click(object sender, EventArgs e)
      {
         string msg = cmdOptions.Text;
         if (msg == "") 
            return;
         if (!cmdOptions.Items.Contains(msg))
         {
            cmdOptions.Items.Add(msg);
            cmdOptions.SelectedItem = msg;
         }
         else
            cmdOptions.SelectedItem = msg;
         
         bool alreadyAtEnd = false;
         if (mCmdHistory.Count > 0)
         {
            if (mCmdHistory[mCmdHistory.Count - 1].Equals(msg))
               alreadyAtEnd = true;
         }
         
         if (!alreadyAtEnd)
         {
            mCmdHistory.Add(msg);
            if (mCmdHistory.Count > 64)
               mCmdHistory.RemoveAt(0);
         }
         mCmdHistoryIndex = mCmdHistory.Count;          
         

         // CVD [6.6.08] : Lets move the last used command to the top of the list
         String lastUsed = cmdOptions.SelectedItem.ToString();
         for (int i = cmdOptions.SelectedIndex; i > 0; i--)
         {
            cmdOptions.Items[i] = cmdOptions.Items[i - 1];
         }
         cmdOptions.Items[0] = lastUsed;
         cmdOptions.SelectedIndex = cmdOptions.Items.IndexOf(lastUsed);
                           
         cmdOutput.Items.Add("> " + msg);
         //string rsp="";
         
         
/*
         XenonInterface.eErrorType res = XenonInterface.sendCommand(msg, out rsp);

         if (res != XenonInterface.eErrorType.cOK)
            cmdOutput.Items.Add(">> Send failed!");
         else if (rsp != "200- OK")
            cmdOutput.Items.Add(">> " + rsp);
 */

         if(msg[0] == '[')
         {
            int idx = msg.LastIndexOf(']');
            if (idx != -1)
            {
               string tgt = msg.Substring(1, idx - 1);
               msg = msg.Remove(0, tgt.Length + 2);
               msg.Trim();

               if (msg.ToLower().StartsWith("runcmdscript"))
               {
                    int startIdx = msg.IndexOf("(") + 2;
                    int endIdx = msg.LastIndexOf(")") - 1;
                    ArrayList commands = new ArrayList();
                    loadScriptedCommands(msg.Substring(startIdx, endIdx - startIdx), commands);
                    foreach (String cmd in commands)
                    {
                        cmdOutput.Items.Add("> script executing: " + cmd);
                        DebugConnectionManager.sendCommandToIP(tgt, cmd);
                    }
               }

               DebugConnectionManager.sendCommandToIP(tgt, msg);
            }
         }
         // CVD [6.6.08] : command catch to batch execute script
         else if (msg.ToLower().StartsWith("runcmdscript"))
         {
            int startIdx = msg.IndexOf("(") + 2;
            int endIdx = msg.LastIndexOf(")") - 1;
            ArrayList commands = new ArrayList();
            loadScriptedCommands(msg.Substring(startIdx, endIdx - startIdx), commands);
            foreach (String cmd in commands)
            {
                cmdOutput.Items.Add("> script executing: " + cmd);
                DebugConnectionManager.sendCommand(cmd);
            }
         }
         else
         {
            DebugConnectionManager.sendCommand(msg); 
         }
         
                  
         if (cmdOutput.Items.Count > 0)
         {
         
            cmdOutput.SelectedIndex = cmdOutput.Items.Count - 1;
            cmdOutput.SelectedIndex = -1;
         }

         cmdOptions.Text = "";         
      }
            
      private void cmdOutput_MouseDoubleClick(object sender, MouseEventArgs e)
      {
         if (cmdOutput.Items.Count < 1)
            return;

         int i = cmdOutput.SelectedIndex;
         if (i >= 0)
         {
            string line = cmdOutput.Items[i].ToString();
            if (line.Substring(0, 2) != @">>" && line.Substring(0, 2) != @"--")
            {
               if ((line.Length > 2) && (line[0] == '>'))
                  cmdOptions.Text = line.Substring(2);
               else
                  cmdOptions.Text = line;
            }
         }
      }

      private void cmdLinePage_Click(object sender, EventArgs e)
      {
      }

      private void cmdOptions_SelectedIndexChanged(object sender, EventArgs e)
      {
      }
            
      private void cmdOptions_KeyUp(object sender, KeyEventArgs e)
      {
         if (e.KeyCode == Keys.Enter)
            cmdSubmit_Click(sender, null);
      }

      private void cmdOptions_SelectionChangeCommitted(object sender, EventArgs e)
      {
      }

      private void serverIPDropDown_SelectedIndexChanged(object sender, EventArgs e)
      {
         if(serverIPDropDown.Text != Server.mIP)
         {
            resetServer();
            
            Server.shutdown();
            Server.setup(serverIPDropDown.Text);
            Server.start();

            DebugConnectionManager.shutdown();
            DebugConnectionManager.setup();
            DebugConnectionManager.start();
            
            writeOptions(true);
         }
      }

      private void serverIPDropDown_TextUpdate(object sender, EventArgs e)
      {
         serverIPDropDown.Text = Server.mIP;
      }

      private void screenshotButton_Click(object sender, EventArgs e)
      {
         DebugConnectionManager.sendCommand("screenshot");

         Process.Start(Directory.GetCurrentDirectory() + "\\screenshots");
      }
           
      private void QuickViewButton_Click(object sender, EventArgs e)
      {
         DebugConnectionManager.sendCommand("loadScenario(\"quickView\\quickview\")");
      }
      
      private void disableServerTimeoutCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         writeOptions(false);
         Server.mDisableTimeout=disableServerTimeoutCheckBox.Checked;
      }

      private void DisableUIButton_Click(object sender, EventArgs e)
      {
         DebugConnectionManager.sendCommand("configToggle(\"disableUI\")");
      }

      private void monitorFileChangesCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         writeOptions(false);
         FileMonitor.SetEnabled(monitorFileChangesCheckBox.Checked);
      }

      private void disableAutoResetCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         writeOptions(false);
      }

      private void enableArchivesCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         writeOptions(false);
      }

      private void disableLooseFilesCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         writeOptions(false);
      }

      private void chooseDvdEmuDirButton_Click(object sender, EventArgs e)
      {
         pickDvdEmuDirectory();
      }

      private void copyCinematicsButton_Click(object sender, EventArgs e)
      {
         copyCinematics();
      }

      private void cmdOptions_KeyPress(object sender, KeyPressEventArgs e)
      {

      }

      private void cmdOptions_KeyDown(object sender, KeyEventArgs e)
      {
         if (mCmdHistory.Count > 0)
         {
            if (e.KeyCode == Keys.Up)        
            {
               mCmdHistoryIndex--;
               
               if (mCmdHistoryIndex >= mCmdHistory.Count)
                  mCmdHistoryIndex = 0;
               else if (mCmdHistoryIndex < 0)
                  mCmdHistoryIndex = mCmdHistory.Count - 1;

               cmdOptions.Text = (String)mCmdHistory[mCmdHistoryIndex];               
               e.Handled = true;
            }
            else if (e.KeyCode == Keys.Down)
            {
               mCmdHistoryIndex++;
               
               if (mCmdHistoryIndex >= mCmdHistory.Count)
                  mCmdHistoryIndex = 0;
               else if (mCmdHistoryIndex < 0)
                  mCmdHistoryIndex = mCmdHistory.Count - 1;
                  
               cmdOptions.Text = (String)mCmdHistory[mCmdHistoryIndex];
               e.Handled = true;
            }
         }            
      }

      private void cmdOptions_PreviewKeyDown(object sender, PreviewKeyDownEventArgs e)
      {

      }

      private void button1_Click(object sender, EventArgs e)
      {
         Process notepad = new Process();
         
         notepad.StartInfo.FileName = "notepad.exe";
         notepad.StartInfo.Arguments = String.Concat(workDirTextBox.Text, "\\startup\\user.cfg");
         
         notepad.Start();
      }
   }
}