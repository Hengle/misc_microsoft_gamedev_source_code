using System;
using System.Collections.Generic;
using System.Text;

using System.Collections;
using System.IO;
using System.Diagnostics;
using System.Threading;
using System.Data.SqlClient;


namespace PhoenixBuildServer
{
   class Task
   {
      //-- Adding comment here.
      public enum TaskType
      {
         BatchFile = 0,
         TypeTotal
      };

      #region Properties
      public int ID
      {
         get
         {
            return mID;
         }
         set
         {
            mID = value;
         }
      }
      public string Name
      {
         get
         {
            return mName;
         }
         set
         {
            mName = value;
         }
      }
      public string BatchFileName
      {
         get
         {
            return mBatFilename;
         }
         set
         {
            mBatFilename = value;
         }
      }
      public StreamWriter Log
      {
         get
         {
            return mTaskLog;
         }
         set
         {
            mTaskLog = value;
         }
      }
      public ArrayList Arguments
      {
         get
         {
            return mArgs;
         }
         set
         {
            mArgs = value;
         }
      }
      public ArrayList ProcessKillList
      {
         get
         {
            return mProcessKillList;
         }
         set
         {
            mProcessKillList = value;
         }
      }
      public bool UseExitCodes
      {
         get
         {
            return mUseExitCodes;
         }
         set
         {
            mUseExitCodes = value;
         }
      }
      #endregion


      #region Protected Members

      protected int mID = -1;
      protected StreamWriter mTaskLog;
      protected string mName;
      protected string mBatFilename;
      protected Timer mQuitTimer = null;
      protected bool mUserHasQuit = false;
      protected bool mCheckingForQuit = false;
      protected Process myProcess = null;
      protected ArrayList mArgs;
      protected ArrayList mProcessKillList;
      protected bool mEnableQuitTimer = false;
      protected bool mUseExitCodes = true;

      #endregion


      #region Protected Functions

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
      /// Logs text to the Task's log file.
      /// </summary>
      /// <param name="logText">The text to output to the log file.</param>
      public void logTask(string logText)
      {
         string str = "(TASK) " + logText;
         log(str, mTaskLog);

         Globals.MainForm.logConsole_Async(str);
      }

      protected QUITCODE startProcess(Process myProcess)
      {
         QUITCODE bSuccess = QUITCODE.NONE;

         try
         {
            // Create a timer that waits one second, then invokes every second.
            if (mEnableQuitTimer)
            {
               // Create the delegate that invokes methods for the timer.
               TimerCallback timerDelegate = new TimerCallback(CheckForQuit);

               mQuitTimer = new Timer(timerDelegate, this, 0, 1000);
            }


            myProcess.Start();

            // Start the asynchronous read of the output stream.
            myProcess.BeginOutputReadLine();

            while (myProcess.HasExited == false && mUserHasQuit == false)
            {
               Thread.Sleep(1);
            }


            if(myProcess.HasExited)
            {
               myProcess.WaitForExit();

               if (mUseExitCodes)
               {
                  if (myProcess.ExitCode != 0)
                  {
                     bSuccess = QUITCODE.ERROR;
                  }
               }
            }
            else if(mUserHasQuit == true)
            {
               logTask(" - user quit the task " + mName);
               if (myProcess.HasExited == false)
               {
                  logTask(" - killing task " + mName);
                  
                  myProcess.Kill();
                  myProcess.WaitForExit();

                  //-- Also kill any process that the task specifies.
                  foreach (string processName in mProcessKillList)
                  {
                     Process[] myProcesses;
                     myProcesses = Process.GetProcessesByName(processName);
                     foreach (Process proc in myProcesses)
                     {
                        proc.Kill();
                     }
                  }
               }

               bSuccess = QUITCODE.USERQUIT;
            }

            myProcess.Close();



            // Kill the timer if it has been enabled
            if (mQuitTimer != null)
            {
               mQuitTimer.Dispose();
               mQuitTimer = null;
            }
         }
         catch (Exception exp)
         {
            logTask("ERROR: (startProcess) " + exp.Message);
            bSuccess = QUITCODE.ERROR;
         }

         string successString = "SUCCESS";
         switch(bSuccess)
         {
            case QUITCODE.NONE:
               successString = "SUCCESS";
               break;
            case QUITCODE.ERROR:
               successString = "ERROR";
               break;
            case QUITCODE.USERQUIT:
               successString = "USER QUIT";
               break;
         }

         logTask(" - task has ended. (" + mName + ")" + " (returned: " + successString + ")");
         return (bSuccess);
      }
      #endregion

      #region Public Functions
      public Task(bool enableQuitTimer)
      {
         mEnableQuitTimer = enableQuitTimer;
         mArgs = new ArrayList();
         mProcessKillList = new ArrayList();
      }

      virtual public QUITCODE run()
      {
         QUITCODE bSuccess = QUITCODE.NONE;

         logTask("Begin task " + mName);

         try
         {
            logTask(" - run " + mBatFilename);
            myProcess = new Process();
            myProcess.StartInfo.FileName = mBatFilename;

            // Set working directory if executable name not relative
            if (Path.IsPathRooted(mBatFilename))
            {
                myProcess.StartInfo.WorkingDirectory = Path.GetDirectoryName(mBatFilename);
            }

            foreach (string arg in mArgs)
            {
               myProcess.StartInfo.Arguments = myProcess.StartInfo.Arguments + " " + arg;
            }

#if false
            myProcess.StartInfo.UseShellExecute = true;
#else
            myProcess.StartInfo.CreateNoWindow = true;
            myProcess.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
            myProcess.StartInfo.UseShellExecute = false;
            myProcess.StartInfo.RedirectStandardOutput = true;
            myProcess.StartInfo.RedirectStandardInput = true;
            myProcess.StartInfo.RedirectStandardError = true;

             // Set our event handler to asynchronously read the sort output.
            myProcess.OutputDataReceived += new DataReceivedEventHandler(OutputHandler);
#endif
            
            bSuccess = startProcess(myProcess);
         }
         catch (Exception exp)
         {
            logTask("ERROR: " + exp.Message + " " + mBatFilename);
            bSuccess = QUITCODE.ERROR;
         }

         return (bSuccess);
      }



      static void CheckForQuit(Object state)
      {
         Task task = (Task)state;
         if (task.mCheckingForQuit == true || task.mQuitTimer == null)
            return;

         task.mCheckingForQuit = true;

         //-- Check DB for quit.
         try
         {
            string query = "SELECT * FROM " + Globals.Config.getString(Globals.Config.cDBQueueTableName) + " WHERE running > '1'";
            string query2 = "SELECT * FROM " + Globals.Config.getString(Globals.Config.cDBQueueTableName);

            SqlCommand cmd = new SqlCommand();
            cmd.CommandText = query;
            cmd.Connection = Database.DBConnection;
            SqlCommand cmd2 = new SqlCommand();
            cmd2.CommandText = query2;
            cmd2.Connection = Database.DBConnection;
            Database.DBConnection.Open();
            SqlDataReader rs = cmd.ExecuteReader();
            task.mUserHasQuit = rs.HasRows;
            rs.Close();

            if (task.mUserHasQuit == false)
            {
               //-- if there are no rows, then its for sure cancelled.
               SqlDataReader rs2 = cmd2.ExecuteReader();
               task.mUserHasQuit = !rs2.HasRows;
               rs2.Close();
            }

            Database.DBConnection.Close();
         }
         catch (Exception exp)
         {
            Console.WriteLine("CheckForQuit: error hitting the DB: " + exp.Message);
         }

         //-- If quiting, then
         if (task.mUserHasQuit == true)
         {
            task.mQuitTimer.Dispose();
            task.mQuitTimer = null;
         }

         task.mCheckingForQuit = false;
      }
      #endregion


      private void OutputHandler(object sendingProcess, DataReceivedEventArgs outLine)
      {
         // Collect the sort command output.
         if (!String.IsNullOrEmpty(outLine.Data))
         {
            Console.WriteLine(outLine.Data);
            logTask(outLine.Data);
         }
      }
   }

}
