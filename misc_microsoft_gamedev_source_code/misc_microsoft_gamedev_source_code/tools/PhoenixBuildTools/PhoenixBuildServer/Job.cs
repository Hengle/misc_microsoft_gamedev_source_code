using System;
using System.Collections.Generic;
using System.Text;

using System.Collections;
using System.IO;
using System.Diagnostics;
using System.Threading;
using System.Data;
using System.Data.SqlClient;

namespace PhoenixBuildServer
{
   #region Public Enums

   public enum SYNC
   {
      DIE = -1,
      START = 0,
      UPDATED_START_SYNC = 1,
      UPDATED_EXESPASS = 2,
      UPDATED_BUILDNUMBER = 3,
      UPDATED_EXESCOPIED = 4,
      DONE_WATSON_PREP = 5,
      DONE = 10,
      NumProperties = 7
   };

   public enum QUITCODE
   {
      NONE = 1,
      USERQUIT = 2,
      ERROR = 3
   };

   #endregion


   class Job
   {
      #region Protected Members

      protected string mName;

      protected StreamWriter mJobLog;
      protected string mLogFileName;
      protected QUITCODE mQuitCode = QUITCODE.NONE;
      protected bool mEnableTaskQuitCheck = false;       // false for now.

      #endregion

      #region Properties

      public string Name
      {
         get
         {
            return mName;
         }
      }
      public string LogFileName
      {
         get
         {
            return mLogFileName;
         }
      }
      public StreamWriter Log
      {
         get
         {
            return mJobLog;
         }
      }
      public QUITCODE QuitCode
      {
         get
         {
            return mQuitCode;
         }
      }
      #endregion

      public Job()
      {
      }

      public void End()
      {
         if (mJobLog != null)
            mJobLog.Close();

         //Globals.DBConnection.Close();

         //-- delete the logs files.
         Process cleanupProcess = new Process();
         cleanupProcess.StartInfo.FileName = Globals.Config.getString(Globals.Config.cCleanupBatchFile);
         cleanupProcess.StartInfo.WindowStyle = ProcessWindowStyle.Normal;
         cleanupProcess.StartInfo.UseShellExecute = true;
         cleanupProcess.Start();
      }

      public void init(string name)
      {
         mName = name;

         //-- open log file
         mLogFileName = "log.txt";

         //-- append .txt if the name doesn't have one already.
         if (mLogFileName.EndsWith(".txt") == false)
         {
            mLogFileName += ".txt";
         }

         //-- prepend the location.
         mLogFileName = Globals.Config.getString(Globals.Config.cLogRoot) + mLogFileName;

         //-- Create subdirs if necessary.
         DirectoryInfo info = Directory.GetParent(mLogFileName);
         if (Directory.Exists(info.FullName) == false)
         {
            Directory.CreateDirectory(info.FullName);
         }
         mJobLog = File.AppendText(mLogFileName);
      }

      protected QUITCODE getQuitCode()
      {
         QUITCODE quitCode = QUITCODE.NONE;

         //-- Check DB for quit.
         try
         {
            string query = "SELECT * FROM " + Globals.Config.getString(Globals.Config.cDBQueueTableName) + " WHERE running > '1'";
            SqlDataReader rs = Database.exSQL(query, false);
            if (rs.Read() == false)
            {
               quitCode = QUITCODE.NONE;
            }
            else
            {
               string quit = (string)rs["running"];
               quit.Trim();
               int qt = System.Convert.ToInt32(quit);
               if (qt == 2)
                  quitCode = QUITCODE.USERQUIT;
               else if (qt == 3)
                  quitCode = QUITCODE.ERROR;
               else
                  quitCode = QUITCODE.NONE;

               logJob("QuitCode: " + quitCode.ToString());
            }
            rs.Close();
            Database.DBConnection.Close();
         }
         catch (Exception exp)
         {
            Console.WriteLine("CheckForQuit: error hitting the DB: " + exp.Message);
         }

         return (quitCode);
      }


      protected void setRunStatus(QUITCODE quitCode)
      {
         try
         {
            logJob("Setting run status");
            string query = "SELECT * From " + Globals.Config.getString(Globals.Config.cDBQueueTableName) + " WHERE running = '1'";
            SqlDataReader rs = Database.exSQL(query, false);
            if (rs.HasRows == true)
            {
               rs.Read();
               int queuenum = (int)rs["queuenum"];
               query = "UPDATE " + Globals.Config.getString(Globals.Config.cDBQueueTableName) +
                  " SET running = '" + (int) quitCode + "', status = 'running' WHERE queuenum = '" + queuenum + "'";
               rs.Close();
               Database.exSQL(query, true);
            }
         }
         catch (Exception exp)
         {
            logJob("ERROR: setrunstatus: " + exp.Message);
         }
      }


      virtual public QUITCODE run()
      {
         QUITCODE retValue = QUITCODE.NONE;

         logJob("starting job " + mName);


         // Renew perforce ticket just in case it has expired
         retValue = loginToPerforce();
         if (retValue != QUITCODE.NONE)
            return (retValue);


         return (retValue);
      }



      public Task runBatch(string exeFile)
      {
         Task task = new Task(mEnableTaskQuitCheck);
         task.Name = "Run " + exeFile;

         // Break exeFile into executable and arguments
         string[] split = exeFile.Split(new Char[] { ' ' });
         foreach (string s in split) 
         {
            if (s.Trim() != "")
            {
               if(task.BatchFileName == null)
                  task.BatchFileName = s.Trim();
               else
                  task.Arguments.Add(s.Trim());
            }
         }

         task.Log = mJobLog;
         return (task);
      }


      protected QUITCODE loginToPerforce()
      {
         try
         {
            logJob("Perfoce login");

            QUITCODE retValue = QUITCODE.NONE;

            string exeFile = Globals.Config.getString(Globals.Config.cPerforceLoginBatchFile) + " " +
                              Globals.Config.getString(Globals.Config.cPerforceUser) + " " +
                              Globals.Config.getString(Globals.Config.cPerforceClient);
            Task task = runBatch(exeFile);
            retValue = task.run();

            // If error we must write to the db.
            if (retValue == QUITCODE.ERROR)
            {
               logJob("ERROR: Error login in to Perforce.  Unable to renew ticket");
               setRunStatus(QUITCODE.ERROR);
            }
            return (retValue);
         }
         catch (Exception exp)
         {
            logJob("ERROR: Exception while login in to Perforce: " + exp.Message);
         }
         return (QUITCODE.ERROR);
      }


      // Updates the buildnumber of the top recod in the queue, the record that is "running"
      protected bool updateBuildNumber(string nextBuildNumber)
      {
         try
         {
            logJob("Updating build num");
            string query = "UPDATE " + Globals.Config.getString(Globals.Config.cDBQueueTableName) +
               " SET buildnum = '" + nextBuildNumber + "' WHERE running = '1'";
            Database.exSQL(query, true);
            return (true);
         }
         catch (Exception exp)
         {
            logJob("ERROR: UpdateBuildNum: " + exp.Message);
         }
         return (false);
      }

      /*
      protected bool doVersion(string filename, string version)
      {
         try
         {
            logOutput("Versioning " + filename + " with " + version);
            logDB("Versioning " + filename + " with " + version);

            Task task = runBatch("bat\\version.bat");
            task.Arguments.Add(filename);
            task.Arguments.Add(version + "UO");
            if (task.run() == false)
            {
               return (false);
            }
            return (true);
         }
         catch (Exception exp)
         {
            logOutput("ERROR: VersionExe: " + exp.Message);
         }
         return (false);
      }
      */


      #region Log Functions
      /// <summary>
      /// Open's log file and write output
      /// </summary>
      /// <param name="logMessage">The text to add to the logfile</param>
      /// <param name="w">The log file to write to.</param>
      public static void log(string logMessage, TextWriter w)
      {
         w.WriteLine("{0} : {1} : {2}", DateTime.Now.ToShortDateString(), DateTime.Now.ToLongTimeString(), logMessage);
         // Update the underlying file.
         w.Flush();
      }

      /// <summary>
      /// Logs text to the Job's log file.
      /// </summary>
      /// <param name="logText">The text to output to the log file.</param>
      public void logJob(string logText)
      {
         string str = " (JOB) " + logText;
         log(str, mJobLog);

         Globals.MainForm.logConsole_Async(str);
      }

      #endregion

   }
}
