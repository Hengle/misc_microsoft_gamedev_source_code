using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Threading;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows.Forms;
using System.Text.RegularExpressions;

using Microsoft.Win32;
using System.Reflection;

using LightingClient;

namespace EditorCore
{


   public class PerforceSimple
   {
      private bool mVerbose;

      private string mLastError;

      public string LastError
      {
         get { return mLastError; }
      }

      public PerforceSimple(bool verbose)
      {
         mLastError = "";
         mVerbose = verbose;
      }

      public bool Checkout(string filename)
      {
         string command = "-s edit " + filename;
         return ExecuteCommand(command);
      }

      public bool Lock(string filename)
      {
         string command = "-s lock " + filename;
         return ExecuteCommand(command);
      }

      public bool Revert(string filename)
      {
         string command = "-s revert " + filename;
         return ExecuteCommand(command);
      }

      public bool Sync(string filename)
      {
         string command = "-s sync -f " + filename;
         return ExecuteCommand(command);
      }

      public bool Checkin(string filename, string description)
      {
         string command = "-s submit -d \"" + description + "\" " + filename;
         return ExecuteCommand(command);
      }


      protected bool ExecuteCommand(string command)
      {
         Process proc = new Process();
         proc.StartInfo.UseShellExecute = false;
         proc.StartInfo.RedirectStandardOutput = true;
         //            proc.StartInfo.RedirectStandardInput = true;
         proc.StartInfo.RedirectStandardError = true;
         proc.StartInfo.CreateNoWindow = true;
         proc.StartInfo.FileName = @"p4.exe";
         proc.StartInfo.Arguments = (command);
         proc.Start();

         proc.WaitForExit();

         string stdOutput = "";
         string stdError = "";

         // grab std out and std err
         stdOutput = proc.StandardOutput.ReadToEnd();
         stdError = proc.StandardError.ReadToEnd();

         if (mVerbose)
         {
            Console.WriteLine(stdOutput);
         }
         Console.WriteLine(stdError);

         // did we have an error?
         if (stdError.Length > 0)      // std error has data
         {
            mLastError = stdError;
            return false;
         }

         // std out starts with "error:"
         if (stdOutput.ToLower().StartsWith("error:"))
         {
            mLastError = stdOutput;
            return false;
         }

         mLastError = "";

         return true;
      }

   }


   /// <summary>
   /// 
   /// </summary>
   public class PerforceManager
   {
      PerforceConnection mConnection = new PerforceConnection();

      public PerforceChangeList GetNewChangeList()
      {
         return GetNewChangeList("editor automatic checkout");
      }
      public PerforceChangeList GetNewChangeList(string description)
      {
         PerforceChangeList newChangeList = null;
         int id = mConnection.CreateChangeList(description);
         if (id != -1)
         {
            newChangeList = new PerforceChangeList(id);
         }

         return newChangeList;
      }
      public PerforceChangeList GetExistingChangeList(int id)
      {
         PerforceChangeList newChangeList = null;
         if (id != -1)
         {
            newChangeList = new PerforceChangeList(id);
            newChangeList.UpdateChangeListInfo();
         }

         return newChangeList;
      }

      public bool HasFilesOpen(int changelistID)
      {
         string results = TerrainGlobals.getPerforce().getConnection().P4Cmd("opened -c " + changelistID.ToString(), "", "");
         if (TerrainGlobals.getPerforce().getConnection().mLastError.Contains("not opened"))
         {
            return false;
         }
         return true;
      }

      public void CleanEmptyChangeLists(string prefix)
      {


         Dictionary<int, string> lists = TerrainGlobals.getPerforce().GetCurrentUserChangelists();
         Dictionary<int, string>.Enumerator it = lists.GetEnumerator();

         while (it.MoveNext())
         {
            if (it.Current.Value.StartsWith(prefix))
            {
               if (TerrainGlobals.getPerforce().HasFilesOpen(it.Current.Key) == false)
               {
                  //TerrainGlobals.ShowMessage("Deleting empty changelist:" + it.Current.Value);
                  TerrainGlobals.getPerforce().getConnection().P4DeleteList(it.Current.Key);
               }

            }

         }

      }

      //Change 35576 on 2007/07/10 by Pthomas@SYS487 *pending* 'Auto import gr2: D:\x\work\art\'
      //List<Pair<string,int>> 
      public Dictionary<int, string> GetCurrentUserChangelists()
      {
         Dictionary<int, string> lists = new Dictionary<int, string>();
         string userName;

         if (TerrainGlobals.getPerforce().getConnection().mSettings.TryGetValue("User name", out userName))
         {
            string results = TerrainGlobals.getPerforce().getConnection().P4Cmd("changes -l -s pending -u " + userName, "", "");

            StringReader res = new StringReader(results);

            string line = res.ReadLine();
            while (line != null && line != "")
            {
               string[] tokens = line.Split(' ');
               int id = -1;
               int.TryParse(tokens[1], out id);
               res.ReadLine();  //blank
               string name = res.ReadLine().Trim();
               //string name = line.Substring(1 + line.IndexOf('\''), -2 + line.Length - line.IndexOf('\''));
               lists[id] = name;

               //res.ReadLine();  //blank
               //line = res.ReadLine();
               line = res.ReadLine();
               while (line != null)// && line != "" )
               {

                  if (line.StartsWith("Change"))
                  {
                     break;  //next entry
                  }
                  else
                  {
                     //could appened to description.. but that doesn't matter to us
                  }
                  line = res.ReadLine();
               }
            }
         }

         return lists;
      }
      //UpdateChangeListInfo()
      //Remove and revert?




      List<PerforceChangeList> mChangeLists = new List<PerforceChangeList>();
      public ICollection<PerforceChangeList> ChangeLists
      {
         get
         {
            return mChangeLists;
         }
      }
      public PerforceConnection getConnection() { return mConnection; }


      public SimpleFileStatus GetFileStatusSimple(string filename)
      {
         return new SimpleFileStatus(filename, mConnection.P4GetFileStatus(filename));
      }
   }

   public enum ePerforceAction
   {
      None,
      Add,
      Edit,
      Delete,
      Branch,
      Integrate
   }

   public class PerforceChangeList
   {
      Regex fileListEX = new Regex(@"(?<filename>//.+)\t[#]\s(?<status>\S+)");
      Regex chageFileEX = new Regex(@"(?<header>[#]\s\sFiles:.*Change:\s*)(?<change>.*)Date:(?<date>.*)Client:(?<client>.*)User:(?<user>.*)Status:(?<status>.*)Description:((?<description>.*?)Files:(?<files>.*)|(?<description>.*))", RegexOptions.Singleline);

      //(?<key>\b.*?):(\t|[\r\n\t]*)(?<value>\b.*\b)
      public class P4Resource
      {
         public string mLocalName = "";
         public string mPerforceName = "";
         public ePerforceAction mPerforceAction = ePerforceAction.None;
         private PerforceChangeList mList = null;

         public Dictionary<string, string> mStatus = null;
         public P4Resource(PerforceChangeList list, string localName, string perforceName, string perforceAction)
         {
            mList = list;
            mLocalName = localName;
            mPerforceName = perforceName;
            mPerforceAction = (ePerforceAction)Enum.Parse(typeof(ePerforceAction), perforceAction, true);

            mStatus = TerrainGlobals.getPerforce().getConnection().P4GetFileStatus(mPerforceName);

         }
         public bool Revert()
         {
            return TerrainGlobals.getPerforce().getConnection().P4Revert(mList.ID, mPerforceName);
         }
      }

      public string GetLastError()
      {
         return TerrainGlobals.getPerforce().getConnection().mLastError;
      }

      public PerforceChangeList(int id)
      {
         mID = id;

         mbListAlive = true;
      }
      private int mID = -1;
      public int ID
      {
         get
         {
            return mID;
         }
      }
      private string mDescription = "";
      public string Description
      {
         set
         {
            mDescription = value;
            TerrainGlobals.getPerforce().getConnection().P4ChangeDescription(ID, mDescription);
         }
         get
         {
            return mDescription;
         }
      }

      private bool mbKnownDirty = false;
      public bool AddFile(string fileName)
      {


         bool result = TerrainGlobals.getPerforce().getConnection().P4Add(ID, fileName);
         mbKnownDirty = true;
         return result;
      }


      public bool EditFile(string fileName)
      {
         bool result = TerrainGlobals.getPerforce().getConnection().P4Sync(fileName);
         bool result2 = TerrainGlobals.getPerforce().getConnection().P4Checkout(ID, fileName);
         mbKnownDirty = true;
         return result;

      }
      public bool ReOpenFile(string fileName)
      {
         bool result = TerrainGlobals.getPerforce().getConnection().P4Reopen(ID, fileName);
         mbKnownDirty = true;
         return result;
      }
      public bool SyncFile(string fileName)
      {
         return TerrainGlobals.getPerforce().getConnection().P4Sync(fileName);
      }

      public bool AddOrEdit(string fileName)
      {
         return AddOrEdit(fileName, false);
      }

      /// <summary>
      /// Do you want a file in a change list.  This will accomplish the goal in several ways.  
      /// </summary>
      /// <param name="fileName"></param>
      /// <param name="reopen">pulls the file from another one of the user's change list into this one</param>
      /// <returns></returns>
      public bool AddOrEdit(string fileName, bool reopen)
      {
         SimpleFileStatus status = TerrainGlobals.getPerforce().GetFileStatusSimple(fileName);
         if (status.State == eFileState.NotInPerforce)
         {
            return AddFile(fileName);
         }
         //else if (status.IsLatestRevision == false)
         //{
         //   return EditFile(fileName);
         //}
         else if (reopen && status.CheckedOutThisUser == true)
         {
            if (status.UserChangeListNumber != ID)
            {
               return ReOpenFile(fileName);//yoink
            }
            else
            {
               return true;//we already have it!
            }
         }
         else
         {
            return EditFile(fileName);
         }
      }



      public bool RevertFile(string fileName)
      {
         bool result = TerrainGlobals.getPerforce().getConnection().P4Revert(ID, fileName);
         mbKnownDirty = true;
         return result;

      }

      public bool Revert()
      {
         UpdateChangeListInfo();
         bool result = true;
         foreach (P4Resource r in mFiles)
         {
            result &= r.Revert();
         }

         return result;
      }

      bool mbListAlive = false;

      public bool RemoveListAndRevert()
      {
         if (mbListAlive == false)
            return false;
         bool result = true;
         result &= Revert();
         result &= TerrainGlobals.getPerforce().getConnection().P4DeleteList(ID);

         mbListAlive = false;

         return result;
      }

      public bool Submitchanges()
      {

         bool result = TerrainGlobals.getPerforce().getConnection().P4Submit(ID);
         mbKnownDirty = true;
         return result;
      }


      public void UpdateChangeListInfo()
      {
         string changelist = TerrainGlobals.getPerforce().getConnection().P4Cmd("change -o ", ID.ToString(), "");
         ParseChanges(changelist);
         mbKnownDirty = false;
      }

      private void ParseChanges(string changelist)
      {
         mFileNames.Clear();

         Match changeMatch = chageFileEX.Match(changelist);

         string files = changeMatch.Groups["files"].Value;

         mDescription = changeMatch.Groups["description"].Value.Trim();

         MatchCollection fileMatches = fileListEX.Matches(files);
         for (int i = 0; i < fileMatches.Count; i++)
         {
            Match info = fileMatches[i];
            mFiles.Add(new P4Resource(this, "", info.Groups["filename"].Value, info.Groups["status"].Value));
            mFileNames.Add(info.Groups["filename"].Value);
         }
      }
      public List<string> mFileNames = new List<string>();
      public bool HasFiles(List<string> files)
      {
         bool found = false;
         foreach (string file in files)
         {
            found = false;
            foreach (P4Resource r in mFiles)
            {
               if (r.mLocalName == file)
                  found = true;
               else if (r.mPerforceName == file)
                  found = true;
            }
            if (found == false)
               return false;
         }
         return true;
      }


      List<P4Resource> mFiles = new List<P4Resource>();
      public ICollection<P4Resource> Files
      {
         get
         {
            return mFiles;
         }
      }
   }

   public class PerforceConnection
   {
      private string mUsername = null;
      private string mClientname = null;
      private string mClienthost = null;

      public PerforceConnection()
      {

      }

      bool mbSetup = false;

      public Dictionary<string, string> mSettings = new Dictionary<string, string>();
      Regex mInfoEX = new Regex(@"^(?<key>.*?):\s(?<value>.*\b).*$", RegexOptions.Multiline);
      public bool Setup()
      {
         mbSetup = true;
         if (IsPerforceInstalled())
         {
            string output = P4Cmd("info", "");

            //File.WriteAllText("perfoceinfo.txt", output);
            mSettings.Clear();
            MatchCollection matches = mInfoEX.Matches(output);
            foreach (Match m in matches)
            {
               if (m.Success == true)
               {
                  mSettings[m.Groups["key"].Value] = m.Groups["value"].Value;
               }
            }


            if (mSettings.TryGetValue("User name", out mUsername)
            && mSettings.TryGetValue("Client name", out mClientname)
            && mSettings.TryGetValue("Client host", out mClienthost))
            {
               return true;
            }
            else
            {
            //   TerrainGlobals.UsingPerforce = false;
               return false;
            }
         }

        // TerrainGlobals.UsingPerforce = false;
         return false;
      }

      public string P4Cmd(string cmd, string arg)
      {
         return P4Cmd(cmd, arg, "");
      }

      public bool mbThrowStandardErrors = false;
      public string mLastError = "";
      public string P4Cmd(string cmd, string arg, string inputPipe)
      {
         if (mbSetup == false)
         {
            if (Setup() == false)
            {
               return "";
            }
         }
         string output = "";
         mLastError = "";
         try
         {
            PerforceLog(cmd + " " + arg);
            /// <summary> P4Cmd is a method in the Perforce class.
            /// <para> The firs string parameter is the standard P4 command, and the second string parameter contains all the arguments for that command.
            /// </summary>
            Process proc = new Process();
            proc.StartInfo.UseShellExecute = false;
            proc.StartInfo.RedirectStandardOutput = true;
            proc.StartInfo.RedirectStandardInput = true;
            proc.StartInfo.RedirectStandardError = true;
            proc.StartInfo.CreateNoWindow = true;
            proc.StartInfo.FileName = @"p4.exe";
            proc.StartInfo.Arguments = (cmd + " " + arg);
            proc.Start();

            proc.StandardInput.Write(inputPipe);
            proc.StandardInput.Write("\r\n");
            proc.StandardInput.Close();
            //proc.WaitForExit();  
            output = proc.StandardOutput.ReadToEnd();
            string errortext = proc.StandardError.ReadToEnd();
            PerforceLog("stdout: " + output);
            PerforceLog("stderr: " + mLastError);

            if (errortext.Length > 0)
               mLastError = errortext;

            if (mbThrowStandardErrors && errortext.Length > 0)
            {
               //TerrainGlobals.getErrorManager().OnException()
               throw new System.Exception("Perforce Error: (" + mLastError + ")");
            }
         }

         catch (System.Exception ex)
         {
            Console.WriteLine("error using perforce: " + ex.ToString());
         }

         return (output);
      }
      public bool IsPerforceInstalled()
      {
         RegistryKey hklm = Registry.LocalMachine;
         try
         {
            RegistryKey software = hklm.OpenSubKey("SOFTWARE", false);
            RegistryKey perfKey = software.OpenSubKey("perforce", false);
            //needs help
            if (perfKey != null && (perfKey.ValueCount != 0 || perfKey.SubKeyCount != 0))
            {
               return true;
            }
            else
               return false;
         }
         catch
         {
            return false;
         }
      }

      //Change 7834 created.
      Regex createdEX = new Regex(@"Change\s(.*)\screated.*");
      public int CreateChangeList()
      {
         return CreateChangeList("editor automatic checkout");
      }
      public int CreateChangeList(string description)
      {
         string input = WriteChangeFile(mClientname, mUsername, description);
         PerforceLog("temp_changes.txt: " + input);
         string output = P4Cmd("change -i -o", "", input);

         int result = -1;
         Match m = createdEX.Match(output);
         if (m.Success == true)
         {
            result = System.Convert.ToInt32(m.Groups[1].ToString());
         }
         return result;
      }

      public static int cDefaultChangeList = -6969696;

      private string WriteChangeFile(string p4Clientname, string p4Username, string p4Description)
      {
         return string.Format("Change:\tnew\r\n\r\nClient:\t{0}\r\n\r\nUser:\t{1}\r\n\r\nStatus:\tnew\r\n\r\nDescription:\r\n\t{2}\r\n\r\nFiles:\r\n", p4Clientname, p4Username, p4Description);
      }

      public bool P4Checkout(int changeListId, string filename)
      {
         string changeliststring = (changeListId == cDefaultChangeList) ? "default" : changeListId.ToString();
         string output = P4Cmd("edit -c " + changeliststring, "\"" + filename + "\"");
         return true;
      }

      //revert -c 7841 //depot/phoenix/xbox/work/scenario/e1/e1.gls
      //stdout: //depot/phoenix/xbox/work/scenario/e1/e1.gls#none - was add, abandoned
      public bool P4Revert(int changeListId, string filename)
      {
         string changeliststring = (changeListId == cDefaultChangeList) ? "default" : changeListId.ToString();

         string output = P4Cmd("revert -c " + changeliststring, "\"" + filename + "\"");

         return true;
      }

      public bool P4Sync(string filename)
      {
         string output = P4Cmd("sync ", "\"" + filename + "\"");
         return true;
      }

      public bool P4DeleteList(int changeListId)
      {
         string changeliststring = (changeListId == cDefaultChangeList) ? "default" : changeListId.ToString();
         string output = P4Cmd("change -d " + changeliststring, "");
         return true;
      }
      public bool P4Submit(int changeListId)
      {
         string changeliststring = (changeListId == cDefaultChangeList) ? "default" : changeListId.ToString();
         string output = P4Cmd("submit -c " + changeliststring, "");
         return true;
      }

      Regex addEX = new Regex(@"(.*)\s-\sopened\sfor\sadd");
      public bool P4Add(int changeListId, string filename)
      {
         string changeliststring = (changeListId == cDefaultChangeList) ? "default" : changeListId.ToString();

         string output = P4Cmd("add -c " + changeliststring + " -f", "\"" + filename + "\"");

         int result = -1;
         Match m = addEX.Match(output);
         if (m.Success == true)
         {
            string f = m.Groups[1].ToString();
            return true;
         }
         return false;
      }

      public bool P4Reopen(int changeListId, string filename)
      {
         string changeliststring = (changeListId == cDefaultChangeList) ? "default" : changeListId.ToString();

         string output = P4Cmd("reopen -c " + changeliststring, "\"" + filename + "\"");

         return true;

      }

      //Change 7841 updated.
      public bool P4ChangeDescription(int changeListId, string description)
      {
         string changeliststring = (changeListId == cDefaultChangeList) ? "default" : changeListId.ToString();


         string input = P4Cmd("change -o " + changeliststring, "");
         string replacement = "${start}" + description + "\r\nFiles:${end}";
         string expression = "(?<start>.*Description:)(?<description>.*)(Files:)?(?<end>.*)";
         string result = Regex.Replace(input, expression, replacement, RegexOptions.Singleline);
         string res2 = P4Cmd("change -i ", "", result);
         return true;
      }


      //C:\Documents and Settings\afoster.ENS>p4 fstat f:/phoenix/xbox/code/tools/PhoenixEditor/Bin/desertworm.jpg
      //f:/phoenix/xbox/code/tools/PhoenixEditor/Bin/desertworm.jpg - no such file(s).
      Regex fileInfoEX = new Regex(@"^(\.\.\.\s\.\.\.\s(?<key>.*?)|\.\.\.\s(?<key>.*?))\s(?<value>.*\b)", RegexOptions.Multiline);
      public Dictionary<string, string> P4GetFileStatus(string filename)
      {
         string status = P4Cmd("fstat", "\"" + filename + "\"");
         Dictionary<string, string> output = new Dictionary<string, string>();

         MatchCollection fileInfo = fileInfoEX.Matches(status);
         for (int i = 0; i < fileInfo.Count; i++)
         {
            Match info = fileInfo[i];
            output[info.Groups["key"].Value] = info.Groups["value"].Value;
         }
         return output;
      }
      public SimpleFileStatus P4GetFileStatusSimple(string filename)
      {
         return new SimpleFileStatus(filename, P4GetFileStatus(filename));
      }


      private void PerforceLog(string text)
      {

         string logfile = Path.Combine(Application.StartupPath, "newPerforceLog.txt");
         if (File.Exists(logfile) == false)
         {
            return;
         }
         //return;
         Debug.Write(text);
         lock (this)
         {
            StreamWriter w = File.AppendText(logfile);
            w.WriteLine(text);
            w.Close();
         }
      }


   }
   public enum eFileState
   {
      NotInPerforce,
      CheckedIn,
      CheckedOutByOther,
      CheckedOutByUser,
      CheckedOutByUserAndOther,
      InvalidState

   }



   public class SimpleFileStatus
   {
      private Dictionary<string, string> mStatus = null;

      string mFileName = "";
      public SimpleFileStatus(string file, Dictionary<string, string> status)
      {
         mStatus = status;


         mFileName = file;


         ComputeState();
      }

      public void Refresh()
      {
         if (mStatus.ContainsKey("depotFile"))
         {
            mStatus = TerrainGlobals.getPerforce().getConnection().P4GetFileStatus(mStatus["depotFile"]);


         }
         else
         {
            mStatus = TerrainGlobals.getPerforce().getConnection().P4GetFileStatus(mFileName);

         }
         ComputeState();

      }
      private void ComputeState()
      {
         mState = eFileState.InvalidState;
         if (InPerforce == false)
         {
            mState = eFileState.NotInPerforce;
         }
         else if (CheckedOutThisUser && CheckedOutOtherUser)
         {
            mState = eFileState.CheckedOutByUserAndOther;
         }
         else if (CheckedOutThisUser)
         {
            mState = eFileState.CheckedOutByUser;
         }
         else if (CheckedOutOtherUser)
         {
            mState = eFileState.CheckedOutByOther;
         }
         else if (CheckedOut == false)
         {
            mState = eFileState.CheckedIn;
         }
      }

      public string DepotFile
      {
         get
         {
            if (mStatus.ContainsKey("depotFile"))
            {

               return mStatus["depotFile"];
            }

            return "";
         }
      }
      public string ClientFile
      {
         get
         {
            if (mStatus.ContainsKey("clientFile"))
            {

               return mStatus["clientFile"];
            }
            if (mStatus.Count == 0)
            {
               return mFileName;
            }


            return "";
         }
      }

      eFileState mState = eFileState.InvalidState;
      public eFileState State
      {
         get
         {
            return mState;

         }

      }



      /// <summary>
      /// Does the file exist in perforce?
      /// </summary>
      public bool InPerforce
      {
         get
         {
            if (mStatus.Count == 0)
            {
               return false;
            }
            else
            {
               //CLM maybe this file was deleted in perforce.. But you're still trying to export it??
               string val = "";
               if (mStatus.TryGetValue("isMapped \r", out val))
               {
                  if (val.Contains("delete"))
                     return false;
               }
               return true;
            }

         }
      }


      public bool CheckedOut
      {
         get
         {
            return CheckedOutOtherUser || CheckedOutThisUser;
         }
      }
      public bool CheckedOutOtherUser
      {
         get
         {
            if (mStatus.ContainsKey("otherOpen0"))
            {
               return true;
            }
            return false;
         }
      }
      public bool CheckedOutThisUser
      {
         get
         {
            if (mStatus.ContainsKey("action") && mStatus.ContainsKey("actionOwner"))
            {
               return true;
            }
            return false;
         }
      }
      public int UserChangeListNumber
      {
         get
         {
            if (CheckedOutThisUser && mStatus.ContainsKey("change"))
            {
               string changelist = mStatus["change"];
               if (changelist == "default")
               {
                  return PerforceConnection.cDefaultChangeList;
               }
               else
               {
                  int listnumber = 0;
                  if (Int32.TryParse(changelist, out listnumber) == true)
                  {
                     return listnumber;
                  }
               }
            }

            return -1;
         }
      }

      public bool IsLatestRevision
      {
         get
         {
            if (mStatus.ContainsKey("headRev") && mStatus.ContainsKey("haveRev"))
            {
               if (mStatus["headRev"] == mStatus["haveRev"])
                  return true;
            }
            return false;
         }
      }
      public bool SyncToLatest()
      {
         return TerrainGlobals.getPerforce().getConnection().P4Sync(mStatus["depotFile"]);

      }


      /// <summary>
      /// This is the owner owns the current action.  "none" if there is no owner.
      /// </summary>
      public string ActionOwner
      {
         get
         {
            if (mStatus.ContainsKey("actionOwner"))
            {
               return mStatus["actionOwner"];
            }
            else if (mStatus.ContainsKey("otherOpen0"))
            {
               return mStatus["otherOpen0"];
            }
            else
            {
               return "none";
            }
         }

      }


   }

}

