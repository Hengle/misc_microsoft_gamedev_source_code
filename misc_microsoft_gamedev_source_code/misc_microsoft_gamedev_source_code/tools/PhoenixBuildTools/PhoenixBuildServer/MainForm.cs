using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using System.Data.SqlClient;
using System.IO;
using System.Threading;
using System.Diagnostics;

using System.Text.RegularExpressions;

namespace PhoenixBuildServer
{
   public partial class MainForm : Form
   {
      private StreamWriter mServerLog;

      //-- What job is running now.
      private int mCurrentJobNum = -1;

      Thread mJobThread = null;


      string mConfigFileName = "config.txt";
      string mLogFileName = "serverlog.txt";

      string mMailPrependString = "PHXBLD - ";

      private string mEmailText;

      public delegate void LogDelegate(string text);
      public LogDelegate logServerDelegate;
      public LogDelegate logConsoleDelegate;


      private int mServerID = -1;


      public MainForm()
      {
         InitializeComponent();

         logServerDelegate = new LogDelegate(logServer);
         logConsoleDelegate = new LogDelegate(logConsole);


         //-- Let's get setup.
         bool success = init();
         if (!success)
         {
            Environment.Exit(0);
         }
      }

      ~MainForm()
      {
         if (mJobThread != null)
         {
            mJobThread.Abort();
         }
      }



      #region Init Functions

      private bool init()
      {
         bool success;


         // First make sure we are only running in the build server machine essd2 if final
         //
         #if(!DEBUG)
            if (Environment.MachineName != "ESSD2" && Environment.MachineName != "SYS600")
            {
               MessageBox.Show(this, "Invalid machine:  " + Environment.MachineName + "\nThis executable is only meant to ran on ESSD2 or SYS600.", "Initilization Failed!", MessageBoxButtons.OK, MessageBoxIcon.Error);
               return (false);
            }

            
            if(Environment.MachineName == "ESSD2")
            {
               mServerID = 1;
            }

            if(Environment.MachineName == "SYS600")
            {
               mServerID = 2;
            }

         #else
            mServerID = 1;
         #endif


         // Init globals
         //
         success = Globals.init(mConfigFileName, this);

         if (!success)
         {
            MessageBox.Show(this, "Unable to initialize globals", "Initilization Failed!", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return (success);
         }

         // Init database
         //
         success = Database.init();

         if (!success)
         {
            MessageBox.Show(this, "Unable to initialize database", "Initilization Failed!", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return (success);
         }

         // Init log
         //

         success = initSiteLog(mLogFileName);

         if (!success)
         {
            MessageBox.Show(this, "Unable to create log file: " + mLogFileName, "Initilization Failed!", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return (success);
         }


         // Init controls
         mServerStatusTextBox.Text = Globals.ServerIsIdleMessage;
         mConsoleStatusTextBox.Text = "";


         logServer("Init success...");
         return (true);
      }


      private bool initSiteLog(string logFileName)
      {
         //-- append .txt if the name doesn't have one already.
         if (logFileName.EndsWith(".txt") == false)
         {
            logFileName += ".txt";
         }

         //-- Create subdirs if necessary.
         DirectoryInfo info = Directory.GetParent(logFileName);
         if (Directory.Exists(info.FullName) == false)
         {
            Directory.CreateDirectory(info.FullName);
         }

         mServerLog = File.AppendText(logFileName);

         return (true);
      }

      #endregion


      #region Log Functions

      /// Open's log file and write output\
      public static void log(string logMessage, TextWriter w)
      {
         w.WriteLine("{0} : {1} : {2}", DateTime.Now.ToShortDateString(), DateTime.Now.ToLongTimeString(), logMessage);
         // Update the underlying file.
         w.Flush();
      }

      /// Logs text to the Site's log file.
      public void logServer(string logText)
      {
         // Make sure we are on the UI thread
         Debug.Assert(this.InvokeRequired == false);

         string str = "(SERVER) " + logText;
         log(str, mServerLog);

         logConsole(str);
      }

      /// Logs text to the console
      public void logConsole(string logText)
      {
         // Make sure we are on the UI thread
         Debug.Assert(this.InvokeRequired == false);

         string str = "(SERVER) " + logText;

         string newColoseText = mConsoleStatusTextBox.Text + logText + "\r\n";

         int max_num_lines = 35;
         newColoseText = cropToLastNumLines(max_num_lines, newColoseText);

         mConsoleStatusTextBox.Text = newColoseText;
      }

      private void logServer_Async(string output)
      {
         this.BeginInvoke(logServerDelegate, output);
      }

      public void logConsole_Async(string output)
      {
         this.BeginInvoke(logConsoleDelegate, output);
      }

      #endregion


      #region Utility funcs

      private static string cropToLastNumLines(int max_num_lines, string text)
      {
         int start = text.Length - 1;
         int at = 0;
         int line_count = 0;
         while ((start > -1) && (at > -1) && (line_count <= max_num_lines))
         {
            at = text.LastIndexOf("\r\n", start);
            if (at > -1)
            {
               start = at - 2;
               line_count++;
            }
         }

         // Crop beginning lines if over the limit
         if (at > -1)
         {
            text = text.Substring((at + 2), text.Length - (at + 2));
         }
         return text;
      }

      private static string replaceLineFeedsForMail(string text)
      {
         text = text.Replace("\r\n", "<BR>");
         text = text.Replace("\n", "<BR>");
         return text;
      }

      #endregion

      #region mail functions

      // The mail template basically is a file embedded with 
      // tags.  The tags can be arranged/formatted in any order,
      // so you could send out mail with changes, then build results,
      // then exceptions, or rearrange the order that they are in the mail.
      // You then insert text into a tag location, so the mail template:
      //
      // <changes>
      // <build>
      // <exceptions>
      //
      // after inserting "no changes" for the changes tag would look like:
      //
      // "no changes"
      // <build>
      // <exception>
      //
      // The template is finalized before it is sent, stripping all tags out
      // of the file.

      // this function takes two globals,the location of the mail log (var MailTemplateReport)and the
      // template (var MailTemplate) , and writes the template to the log
      // this function deletes prior mail reports
      private bool mailTemplateInit()
      {
         try
         {
            //-- init the email from the template file.
            StreamReader rdr = File.OpenText(Globals.Config.getString(Globals.Config.cMailTemplateFile));
            mEmailText = rdr.ReadToEnd();
            rdr.Close();
            return (true);
         }
         catch
         {
         }
         return (false);
      }


      // This function inserts the text from strInsert in the mailtemplate location
      // tagged tagName
      private bool mailTemplateInsert(string strInsert, string tagName)
      {
         if (mEmailText.Length <= 0)
         {
            return (false);
         }

         string searchTag = Globals.Config.getString(Globals.Config.cMailTemplateTagStart) + tagName +
            Globals.Config.getString(Globals.Config.cMailTemplateTagEnd);

         //-- replace if we found the tag.
         if (mEmailText.IndexOf(searchTag) != -1)
         {
            mEmailText = mEmailText.Replace(searchTag, strInsert);
         }
         else //-- else add it to the end.
         {
            mEmailText = mEmailText + "\r\n" + strInsert;
         }

         return (true);
      }

      // This function removes all formatting tags in the mail
      // Should be called right before the mail is sent
      // MailTemplateTagEnd and MailTemplateTagStart surround tags 
      private void mailTemplateFinalize()
      {
         while (1 == 1)
         {
            //-- search for beginning tag.
            int startTagIndex = mEmailText.IndexOf(Globals.Config.getString(Globals.Config.cMailTemplateTagStart));
            if (startTagIndex == -1)
               break;
            int endTagIndex = mEmailText.IndexOf(Globals.Config.getString(Globals.Config.cMailTemplateTagEnd));
            if (endTagIndex == -1)
               break;
            endTagIndex += Globals.Config.getString(Globals.Config.cMailTemplateTagEnd).Length;
            mEmailText = mEmailText.Remove(startTagIndex, endTagIndex - startTagIndex);
         }

         StreamWriter file = File.CreateText(Globals.Config.getString(Globals.Config.cMailTemplateReport));
         file.Write(mEmailText);
         file.Close();
      }


      private bool buildSuccessMail(string buildComment, string logsDest)
      {
         //-- create mail report file
         if (mailTemplateInit() == false)
         {
            return (false);
         }

         //-- add build comment to the build email.
         buildComment = replaceLineFeedsForMail(buildComment);

         //-- add logs
         string buildLogs = "<BR>Check logs at <<a href=\"file:///" + logsDest + "\">" + logsDest + "</a>><BR>";

         mailTemplateInsert(buildComment + "<BR><BR>", "BUILDCOMMENT");
         mailTemplateInsert(buildLogs, "BUILDLOGS"); 

         //-- finailized the email
         mailTemplateFinalize();

         return (true);
      }


      private bool buildErrorMail(string buildComment, string logsDest, string buildErrors)
      {
         //-- create mail report file
         if (mailTemplateInit() == false)
         {
            return (false);
         }

         buildComment = replaceLineFeedsForMail(buildComment);
         buildErrors = replaceLineFeedsForMail(buildErrors);

         //-- add logs
         string buildLogs = "<BR>Check logs at <<a href=\"file:///" + logsDest + "\">" + logsDest + "</a>><BR>";

         mailTemplateInsert(buildComment + "<BR>", "BUILDCOMMENT");
         mailTemplateInsert("<BR>...<BR>" + buildErrors, "BUILDERRORS");
         mailTemplateInsert(buildLogs, "BUILDLOGS"); 

         //-- finailized the email
         mailTemplateFinalize();

         return (true);
      }

      // Send mail will take in a properly formatted to: list with ; delimited
      // and send mail out to the list with the BuildReport, aka Body, then
      // an attachment. The attachment is a list of files to attach.
      private bool sendMail(string ToList, string buildReport, string subjectLine, string from)
      {
         try
         {
            Task task = runBatch(Globals.Config.getString(Globals.Config.cSendMailCmd), false);
            string args = "smtp auth:sspi server:smtphost to:" + ToList + " from:" + from + "@ensemblestudios.com server:" + Globals.Config.getString(Globals.Config.cMailServer) + " \"subject:" + subjectLine + "\" body:" + buildReport +
                        " content-type:text/html";

            task.Arguments.Add(args);

            if (task.run() != QUITCODE.NONE)
            {
               return (false);
            }
            return (true);
         }
         catch
         {
         }

         return (false);
      }

      #endregion


      #region Copy Logs


      // Copies local log files to dest, which should be a directory
      // on the prop site. All logs are in BuildRoot\\logs, and the
      // copy uses RoboCopy
      protected bool propLogs(string src, string dest)
      {
         //logDB("Copying logs to prop:" + dest);
         return (roboCopyFileEx(src, dest, "*.*", "/NP /NJH /NJS"));
      }

      protected bool roboCopyFileEx(string source, string dest, string file, string options)
      {
         try
         {
            Task task = runBatch(Globals.Config.getString(Globals.Config.cRoboCopyExe), false);
            task.Arguments.Add(source);
            task.Arguments.Add(dest);
            task.Arguments.Add(file);
            task.Arguments.Add(options);
            task.UseExitCodes = false;

            if (task.run() != QUITCODE.NONE)
            {
               return (false);
            }
            return (true);
         }
         catch
         {
         }
         return (false);
      }

      #endregion


      private Task runBatch(string exeFile, bool enableQuitTimer)
      {
         Task task = new Task(enableQuitTimer);
         task.Name = "Run " + exeFile;

         // Break exeFile into executable and arguments
         string[] split = exeFile.Split(new Char[] { ' ' });
         foreach (string s in split)
         {
            if (s.Trim() != "")
            {
               if (task.BatchFileName == null)
                  task.BatchFileName = s.Trim();
               else
                  task.Arguments.Add(s.Trim());
            }
         }
         task.Log = mServerLog;
         return (task);
      }

      /// <summary>
      /// Gets a full build number in the form of p.vvyy.mmdd.xxxx
      /// p = the symbol server product ID
      /// vv = the type of build
      /// yy = the year of the build
      /// mm = the month of the build
      /// dd = the day of the build
      /// xxxx = the version number of the build.
      /// </summary>
      /// <param name="buildVersionString"></param>
      /// <param name="buildOptionIndex"></param>
      /// <returns></returns>
      private string getBuildNumber(string buildVersionString, int buildConfigIndex)
      {
         try
         {
            string day = ((int)DateTime.Now.Day).ToString();
            string month = ((int)DateTime.Now.Month).ToString();
            string year = ((int)DateTime.Now.Year).ToString();

            if (DateTime.Now.Day < 10)
            {
               day = "0" + day;
            }
            if (DateTime.Now.Month < 10)
            {
               month = "0" + month;
            }

            string buildOptionString;

            if (buildConfigIndex < 10)
            {
               buildOptionString = "0" + buildConfigIndex;
            }
            else
            {
               buildOptionString = buildConfigIndex.ToString();
            }

            string buildNumber = "1." + year.Substring(2, 2) + "_" + month + "_" + day + "." + buildConfigIndex + "." + buildVersionString;
            return (buildNumber);
         }
         catch
         {
         }
         return (null);
      }



      private void runJob()
      {
         BuildQueueItem currentJobInfo = new BuildQueueItem();
         string results = "No errors.";
         bool bUserHasQuit = false;
         bool bSuccess = false;



         logServer_Async("---------------------------");
         logServer_Async("Begin runJob #" + mCurrentJobNum);



         //-- Get job info
         //
         bSuccess = Database.getJobFromQueue(mCurrentJobNum, ref currentJobInfo);

         if (bSuccess)
         {
            //-- Get build type for current config
            BuildTypeItem builtTypeInfo = new BuildTypeItem();

            bSuccess = Database.getBuildTypeFromTypes(currentJobInfo.buildconfig, ref builtTypeInfo);


            if (bSuccess)
            {
               Job job = null;

               switch (builtTypeInfo.type)
               {
                  case "RunBatchBuild":
                     job = new JobRunBatchBuild(builtTypeInfo.extra);
                     break;
                  default:
                     break;
               }

               if (job != null)
               {
                  // init job
                  job.init(builtTypeInfo.name);


                  logServer_Async("Launching job...");
                  logServer_Async(String.Format("[started by: {0}, job type: {1}]", currentJobInfo.builduser, builtTypeInfo.name));

                  // run the job
                  QUITCODE quitCode = job.run();

                  logServer_Async("Job returned (exitcode: " + quitCode.ToString() + ")");


                  bUserHasQuit = (quitCode == QUITCODE.USERQUIT);

                  string userName = currentJobInfo.builduser;
                  string fullBuildNumber = getBuildNumber(/*getBuildVersion(false)*/currentJobInfo.queuenum.ToString(), currentJobInfo.buildconfig);

                  switch (quitCode)
                  {
                     case QUITCODE.NONE:
                        {
                           //-- prop logs to share
                           logServer_Async("Copying logs...");
                           string logsSrc = Globals.Config.getString(Globals.Config.cLogRoot);
                           string logsDest = Globals.Config.getString(Globals.Config.cLogDest) + fullBuildNumber;
                           bSuccess = propLogs(logsSrc, logsDest);

                           if (!bSuccess)
                           {
                              string errorMsg = "FAILURE: Unable to copy logs";
                              logServer_Async(errorMsg);
                              results = errorMsg;
                              break;
                           }

                           string buildCount = null;
                           string changeListNumber = null;
                           string emailSubjectLine = null;

                           //-- this is a hack here.
                           //-- For archive builds retreive the build number number from the file work\tools\databuild\buildcount.txt and 
                           //   the changelist number from work\tools\databuild\changeListSubmission.txt
                           if ((currentJobInfo.buildconfig == 0) || (currentJobInfo.buildconfig == 18) || (currentJobInfo.buildconfig == 27) || (currentJobInfo.buildconfig == 28))
                           {
                              string archiveBuildPath = Path.GetDirectoryName(builtTypeInfo.extra);

                              if (File.Exists(archiveBuildPath + "\\buildcount.txt") == true)
                              {
                                 StreamReader rdr = File.OpenText(archiveBuildPath + "\\buildcount.txt");
                                 buildCount = rdr.ReadToEnd();
                                 buildCount = buildCount.Trim();
                                 rdr.Close();
                              }

                              if (File.Exists(archiveBuildPath + "\\changeListSubmission.txt") == true)
                              {
                                 StreamReader rdr = File.OpenText(archiveBuildPath + "\\changeListSubmission.txt");

                                 Regex changeList1EX = new Regex(@"^Submitting change (?<changelistnumber>\d*)\.$", RegexOptions.Singleline);

                                 String line;
                                 while ((line = rdr.ReadLine()) != null)
                                 {
                                    MatchCollection matches = changeList1EX.Matches(line);
                                    foreach (Match m in matches)
                                    {
                                       if (m.Success == true)
                                       {
                                          changeListNumber = m.Groups["changelistnumber"].Value;
                                          break;
                                       }
                                    }
                                 }

                                 rdr.Close();
                              }
                           }

                           //-- send email
                           logServer_Async("Sending email...");
                           bSuccess = buildSuccessMail(currentJobInfo.comment, logsDest);

                           if (!bSuccess)
                           {
                              string errorMsg = "FAILURE: Unable to create build success mail";
                              logServer_Async(errorMsg);
                              results = errorMsg;
                              break;
                           }

                           if (String.IsNullOrEmpty(buildCount))
                           {
                              emailSubjectLine = mMailPrependString + "New " + builtTypeInfo.name;
                           }
                           else
                           {
                              if (String.IsNullOrEmpty(changeListNumber))
                              {
                                 emailSubjectLine = mMailPrependString + "New " + builtTypeInfo.name + " (#" + buildCount + ")";
                              }
                              else
                              {
                                 emailSubjectLine = mMailPrependString + "New " + builtTypeInfo.name + " (#" + buildCount + ")" + " (#" + changeListNumber + ")";
                              }
                           }

                           bSuccess = sendMail(Globals.Config.getString(Globals.Config.cBuildAlias), Globals.Config.getString(Globals.Config.cMailTemplateReport), emailSubjectLine, userName);

                           if (!bSuccess)
                           {
                              string errorMsg = "FAILURE: Unable to send build success mail";
                              logServer_Async(errorMsg);
                              results = errorMsg;
                           }
                        }
                        break;
                     case QUITCODE.USERQUIT:
                        // TODO: send build user a msg that their build got canned.
                        break;
                     case QUITCODE.ERROR:
                        {
                           //-- prop logs to share
                           logServer_Async("Copying logs...");
                           string logsSrc = Globals.Config.getString(Globals.Config.cLogRoot);
                           string logsDest = Globals.Config.getString(Globals.Config.cLogDest) + fullBuildNumber;
                           bSuccess = propLogs(logsSrc, logsDest);

                           if (!bSuccess)
                           {
                              string errorMsg = "FAILURE: Unable to copy logs";
                              logServer_Async(errorMsg);
                              results = errorMsg;
                              break;
                           }

                           //-- send email
                           logServer_Async("Sending email...");

                           // Get errors
                           StreamReader rdr = File.OpenText(job.LogFileName);
                           results = rdr.ReadToEnd();
                           rdr.Close();
                           
                           // Only use last 20 lines
                           results = cropToLastNumLines(20, results);

                           bSuccess = buildErrorMail(currentJobInfo.comment, logsDest, results);

                           if (!bSuccess)
                           {
                              string errorMsg = "FAILURE: Unable to create build failure mail";
                              logServer_Async(errorMsg);
                              results = errorMsg;
                              break;
                           }

                           bSuccess = sendMail(Globals.Config.getString(Globals.Config.cFailedBuildAlias), Globals.Config.getString(Globals.Config.cMailTemplateReport), mMailPrependString + "FAILED: " + builtTypeInfo.name + ", from: " + userName, userName);

                           if (!bSuccess)
                           {
                              string errorMsg = "FAILURE: Unable to send build failure mail";
                              logServer_Async(errorMsg);
                              results = errorMsg;
                           }
                        }
                        break;
                  }

                  job.End();
               }
               else
               {
                  string errorMsg = "FAILURE: Build type \"" + builtTypeInfo.type + "\" is not recognized by the build system";
                  logServer_Async(errorMsg);
                  results = errorMsg;
               }
            }
            else
            {
               string errorMsg = "FAILURE: Invalid build configuration id (buildconfig = " + currentJobInfo.buildconfig + ") not in " + Globals.Config.getString(Globals.Config.cDBBuildTypes) + " table";
               logServer_Async(errorMsg); 
               results = errorMsg;
            }
         }
         else
         {
            string errorMsg = "FAILURE: Invalid job number (queuenum = " + mCurrentJobNum + ") not in " + Globals.Config.getString(Globals.Config.cDBQueueTableName) + " table";
            logServer_Async(errorMsg);
            results = errorMsg;
         }

         //-- Enter the history if the user didn't quit.
         if (!bUserHasQuit)
         {
            try// DBHistory contains everything we will insert into the history field
            {
               string vVersion = /*job.getBuildVersion(false) + */"\\";
               string newPath = Globals.Config.getString(Globals.Config.cLogDest) + vVersion;

               if (File.Exists(newPath + Globals.Config.getString(Globals.Config.cHistoryPlaytest)) == true)
               {
                  StreamReader rdr = File.OpenText(newPath + Globals.Config.getString(Globals.Config.cHistoryPlaytest));
                  results = rdr.ReadToEnd() + "\r\n";
               }
               if (File.Exists(newPath + Globals.Config.getString(Globals.Config.cHistoryDebug)) == true)
               {
                  StreamReader rdr = File.OpenText(newPath + Globals.Config.getString(Globals.Config.cHistoryDebug));
                  results = results + rdr.ReadToEnd() + "\r\n";
               }
               if (File.Exists(newPath + Globals.Config.getString(Globals.Config.cHistoryFinal)) == true)
               {
                  StreamReader rdr = File.OpenText(newPath + Globals.Config.getString(Globals.Config.cHistoryFinal));
                  results = results + rdr.ReadToEnd() + "\r\n";
               }
            }
            catch (Exception exp)
            {
               logServer_Async("ERROR: Failed calling OpenText:" + exp.Message);
            }

            //-- Adding to history entry
            logServer_Async("Adding entry to history table...");

            string date = DateTime.Now.ToShortDateString() + " " + DateTime.Now.ToShortTimeString();

            try // InsertHistory can fail for a bunch of reasons, so we catch and insert some default comments
            {
               currentJobInfo.buildnum = currentJobInfo.queuenum.ToString();
               Database.insertHistory(currentJobInfo, currentJobInfo.comment, results, date);
            }
            catch (Exception exp)
            {
               logServer_Async("ERROR: Failed to call insertHistory, possibly too long of a string, will insert default comments:" + exp.Message);
               try
               {
                  Database.insertHistory(currentJobInfo, "Failed to add comments", null, date);
               }
               catch (Exception exp1)
               {
                  logServer_Async("ERROR: Failed to call InsertHistory a second time, comments will be null:" + exp1.Message);
               }
            }
         }

         try // Remove job from queue
         {
            // Remove job from build queue table
            Database.removeJobFromQueue(mCurrentJobNum);
         }
         catch (Exception exp1)
         {
            logServer_Async("ERROR: Failed to call to remove job from the BuildQueue table" + exp1.Message);
         }

         logServer_Async("End job");
         logServer_Async("---------------------------");


         mCurrentJobNum = -1;
      }



      // ---------------------------------
      // Even Handlers
      // ---------------------------------

      private void mStartButton_Click(object sender, EventArgs e)
      {
         if (mBuildTimer.Enabled == true)
         {
            // Stop the timer
            mBuildTimer.Enabled = false;
            mStartButton.Text = "Start";

            if (mJobThread != null)
            {
               mServerStatusTextBox.Text += " - will stop after current job";
            }
         }
         else
         {
            // Start the timer
            mBuildTimer.Enabled = true;
            mStartButton.Text = "Stop";
         }
      }

      private void mBuildTimer_Tick(object sender, EventArgs e)
      {
         if (mJobThread == null)
         {
            // free (look for job)
            int jobNumber = Database.getFirstJobFromQueueAndSetStatus(mServerID);
            if (jobNumber != -1)
            {
               mServerStatusTextBox.Text = Globals.ServerIsBusyMessage + jobNumber;

               //-- tell the app what the current job is.
               mCurrentJobNum = jobNumber;

               mJobThread = new Thread(new ThreadStart(runJob));
               mJobThread.Start();
            }
            else
            {
               mServerStatusTextBox.Text = Globals.ServerIsIdleMessage;
               Database.logDB(mServerStatusTextBox.Text, null, mServerID);
            }
         }
         else
         {
            if (mJobThread.IsAlive)
            {
               // Update DB status
               Database.logDB(mServerStatusTextBox.Text, mConsoleStatusTextBox.Text, mServerID);
            }
            else
            {
               mJobThread = null;
            }
         }
      }
   }

   public class BuildQueueItem
   {
      public int queuenum = -1;
      public string buildnum;
      public string builduser;
      public string timestart;
      public int buildconfig = -1;
      public string buildparams;
      public string comment;
   }

   public class BuildTypeItem
   {
      public int perms = -1;
      public string name;
      public string description;
      public string type;
      public string extra;
   }
}