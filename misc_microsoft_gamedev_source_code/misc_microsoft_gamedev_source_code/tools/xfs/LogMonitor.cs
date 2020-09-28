using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace xfs
{
  

   //============================================================================
   //============================================================================
   //============================================================================
   //============================================================================
   //============================================================================
   //============================================================================
   //============================================================================

   public partial class LogMonitor : UserControl
   {
      private System.Windows.Forms.Timer updateTimer;
      private const int cUpdateInterval = 250;
      static List<logChannel> mLogChannels = new List<logChannel>();



      //commands for the input stream
      static string cAddTag = @"<add>";

      //standard channels
      static string cConsoleTag = @"<console>";
      static string cDebugTag = @"<debug>";
      static string cErrorTag = @"<error>";
      static string cWarningTag = @"<warning>";
      static string cResourceTag = @"<resource>";

      public enum eLogChannels
      {
         cConsoleIndex = -1,
         cTraceIndex = 0,
         cErrorsIndex = 1,
         cWarningsIndex = 2,
         cResourceIndex = 3
      };


      //============================================================================
      // LogMonitor.LogMonitor
      //============================================================================
      public LogMonitor()
      {
         InitializeComponent();

         addChannel("debug");
         addChannel("error");
         addChannel("warning");
         addChannel("resource");
         addChannel("fileManager");

         updateTimer = new System.Windows.Forms.Timer();
         updateTimer.Interval = cUpdateInterval;
         updateTimer.Start();
         updateTimer.Tick += new EventHandler(updateControls);

      }
      //============================================================================
      // LogMonitor.updateControls
      //============================================================================
      private void updateControls(object sender, EventArgs eArgs)
      {
         if (sender == updateTimer)
         {

            for (int i = 0; i < mLogChannels.Count; i++)
            {
               Control[] ctrl = logTabs.TabPages[mLogChannels[i].mLogNum].Controls.Find("logList", true);
               if (ctrl.Length != 0)
               {
                  LogListBox lst = (LogListBox)(ctrl[0]);
                  for (int k = 0; k < mLogChannels[i].mPendingMessages.Count;k++ )
                  {
                     lst.addText(mLogChannels[i].mPendingMessages[k]);
                  }
               }
              
               mLogChannels[i].mPendingMessages.Clear();
            }
            
            
            //now update our tabs
            for (int i = 0; i < logTabs.TabCount; i++)
            {
               Control[] ctrl = logTabs.TabPages[i].Controls.Find("logList", true);
               if (ctrl.Length != 0)
               {
                  LogListBox lst = (LogListBox)(ctrl[0]);
                  lst.updateList();
               }
            }
         }
      }

      //============================================================================
      // LogMonitor.addLogString
      //============================================================================
      private void addLogString(string dat, int tab)
      {
         if (tab < 0)
            Program.mMainForm.addCommandConsoleText(dat);
         else
            mLogChannels[tab].mPendingMessages.Add(dat);
      }

      //============================================================================
      // LogMonitor.addChannel
      //============================================================================
      public int addChannel(string ChannelName)
      {
         //-- Don't add a channel that already exists
         string channelIdent = @"<" + ChannelName + @">";
         foreach (logChannel channel in mLogChannels)
         {
             if (channel.mIdentifier == channelIdent)
                 return -1;
         }

         logTabs.TabPages.Add(ChannelName);

         LogListBox lst0 = new LogListBox();
         lst0.Name = "logList";
         lst0.FormattingEnabled = true;
         lst0.HorizontalScrollbar = true;
         lst0.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
         lst0.SelectionMode = SelectionMode.MultiExtended;
         logTabs.TabPages[logTabs.TabCount-1].Controls.Add(lst0);
         lst0.Dock = DockStyle.Fill;
         

         mLogChannels.Add(new logChannel(logTabs.TabCount - 1, channelIdent));

         return logTabs.TabCount - 1;
      }

      //============================================================================
      // LogMonitor.saveLog
      //============================================================================
      public void saveLog()
      {
         string workDir = Directory.GetCurrentDirectory();
         int k = logTabs.SelectedIndex;
         Control[] ctrl = logTabs.TabPages[k].Controls.Find("logList", true);
         LogListBox lst = (LogListBox)(ctrl[0]);

         //string path = @"xfs_log_" + logTabs.TabPages[k].Text + ".txt";
         SaveFileDialog d = new SaveFileDialog();
         d.Filter = "TXT (.txt)|.txt";
         d.FilterIndex = 0;
         d.InitialDirectory = Directory.GetCurrentDirectory();
         if(d.ShowDialog() == DialogResult.OK)
         {
            string path = d.FileName;
            try
            {
               StreamWriter writer = new StreamWriter(path);
               foreach (string name in lst.Items)
                  writer.WriteLine(name);
               writer.Close();
            }
            catch { }

         }
         Directory.SetCurrentDirectory(workDir);
      }

      //============================================================================
      // LogMonitor.clearLog
      //============================================================================
      public void clearActiveLog()
      {
         int k = logTabs.SelectedIndex;
         Control[] ctrl = logTabs.TabPages[k].Controls.Find("logList", true);
         LogListBox lst = (LogListBox)(ctrl[0]);
         lst.Items.Clear();
      }

      //============================================================================
      // LogMonitor.clearAllLogs
      //============================================================================
      public void clearAllLogs()
      {
         for (int i = 0; i < logTabs.TabCount; i++)
         {
            Control[] ctrl = logTabs.TabPages[i].Controls.Find("logList", true);
            LogListBox lst = (LogListBox)(ctrl[0]);
            lst.Items.Clear();
         }
        
      }

      //============================================================================
      // LogMonitor.giveTabIndex
      //============================================================================
      public int giveTabIndex(string tabName)
      {
         for (int i = 0; i < logTabs.TabCount; i++)
         {
            if(logTabs.TabPages[i].Name == tabName)
               return i;
         }

         return 1;
      }

      //============================================================================
      // LogMonitor.submitMessage
      //============================================================================
      public void submitMessage(string msg,int channel)
      {
         if (msg == null)
            return;

         string inMsg = msg;
         int channelNum = channel;
         if (channelNum == -1)
           channelNum = parseForChannel(ref inMsg);
         
         string[] lst = splitInput(inMsg);

         if (!(lst.Length == 1 && lst[0] == @""))
         {
            for (int i = 0; i < lst.Length; i++)
               addLogString(lst[i], channelNum);
         }
         
         lst = null;
      }
      //============================================================================
      // LogMonitor.splitInput
      //============================================================================

      private string[] splitInput(string dat)
      {
         string b = dat.Replace('\r', ' ');
         b = b.Replace('\t', '\t');
         char[] delim = new char[] { '\n' };
         return b.Split(delim);

      }

      private string[] tokenizeInput(string dat)
      {
         char[] delim = new char[] { ' ',',' };
         return dat.Split(delim);

      }

      private string getCommand(string dat)
      {
         if (dat.Substring(0, 1) != @"<") return cDebugTag;

         int k = dat.IndexOf(@">");
         if (k != -1)
            return dat.Substring(k);

         return dat;
      }

      private string stripCommandOff(string dat)
      {
         if (dat=="" || dat.Substring(0, 1) != @"<") return dat;

         //strip our opening tag
         string tmp = dat;
         int k = dat.IndexOf(@">");
         if (k != -1)
            tmp = dat.Substring(k + 1, dat.Length - (k + 1));

         //strip a closing bracket if there
         k = tmp.IndexOf(@"<");
         if(k!=-1)
            tmp = tmp.Substring(0,k);

         return tmp;
      }

      //============================================================================
      // LogMonitor.parseForChannel
      //============================================================================
      private int parseForChannel(ref string msg)
      {
         int logNum = (int)LogMonitor.eLogChannels.cTraceIndex;

         //check for commands first
         if (msg.StartsWith(cAddTag))
         {
            string chnName = stripCommandOff(msg);
            string[] tokens = tokenizeInput(msg);
            

            //format : <ADD> <channel name> 
            logNum = addChannel(stripCommandOff(tokens[0]));
            msg = @"";
         }
         else
         {
            if (msg.StartsWith("<console>"))
               logNum = -1;
            else
            {
               //we're not a command, we're a msg to go to a channel
               for (int i = 0; i < mLogChannels.Count; i++)
               {
                  if (msg.StartsWith(mLogChannels[i].mIdentifier))
                  {
                     logNum = mLogChannels[i].mLogNum;
                     break;
                  }
               }
            }
            msg = stripCommandOff(msg).Trim();
         }

         return logNum;
      }

      private void logTabs_SelectedIndexChanged(object sender, EventArgs e)
      {

      }
   }

   //============================================================================
   //============================================================================

   public partial class LogListBox : ListBox
   {
      //============================================================================
      // LogListBox.updateList
      //============================================================================
      public void updateList()
      {
         try
         {
            if (mPendingText.Count > 0)
            {
               for (int i = 0; i < mPendingText.Count; i++)
                  Items.Add(mPendingText[i]);
               mPendingText.Clear();
               this.SelectedIndex = Items.Count-1;
               this.SelectedIndex = -1;
            }
         }
         catch { }
      }
      //============================================================================
      // LogListBox.addText
      //============================================================================
      public void addText(string txt)
      {
         mPendingText.Add(txt);
      }
      List<string> mPendingText = new List<string>();

   }


   //============================================================================
   //============================================================================

   public class logChannel
   {
      public logChannel(int logNum, string ident)
      {
         mLogNum = logNum;
         mIdentifier = ident;
      }
      public int mLogNum;
      public string mIdentifier;
      public List<string> mPendingMessages = new List<string>();
   }


}
