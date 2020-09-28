using System;
using System.IO;
using System.Windows.Forms;
using System.Diagnostics;
using EditorCore;
using XDevkit;

//CLM [04.18.06] This file is responsible for interfacing with XFS and the 360.

namespace EditorCore
{
   

   static public class XenonInterface
   {
      public enum eErrorType
      {
         cOK = 0,
         cConnectionLost,
         cCannotConnect,
         cInvalidCommand,
      }

      static public bool init()
      {
         if (mXboxManager == null)
            mXboxManager = new XboxManagerClass();

         if (mDefaultConsole == null)
            mDefaultConsole = mXboxManager.OpenConsole(mXboxManager.DefaultConsole);

         return mDefaultConsole != null;
      }
      static public void destroy()
      {
         mDefaultConsole = null;
         mXboxManager = null;
      }

      static public bool openConnection()
      {
         if (mXboxManager == null || mDefaultConsole == null)
            init();

         try
         {
            mCommandConnectionID = mDefaultConsole.OpenConnection(mCommandStringHandle);
         }
         catch
         {
            mCommandConnectionID = 0;
            CoreGlobals.getErrorManager().OnSimpleWarning("Error quickviewing to 360 : The game did not start properly. Please ensure XFS is running, and that you have the latest versions.");
         }

         return true;
      }
      static public void closeConnection()
      {
         if (mCommandConnectionID != 0)
            mDefaultConsole.CloseConnection(mCommandConnectionID);
      }
      static public void sendCommand(string cmd, out string resp)
      {
         resp = "";
         eErrorType res = eErrorType.cConnectionLost;

         try
         {
            closeConnection();
            openConnection();

            mDefaultConsole.SendTextCommand(mCommandConnectionID, mCommandStringHandle + "!" + cmd, out resp);
            res = eErrorType.cOK;

         }
         catch (Exception e)
         {

            res = errorCatch(e);
         }

      }
      static public void setTextCallback(XboxEvents_OnTextNotifyEventHandler target)
      {
         if(mDefaultConsole!=null)
            mDefaultConsole.OnTextNotify += target;
      }
      static public void setSTDCallback(XboxEvents_OnStdNotifyEventHandler target)
      {
         if (mDefaultConsole != null)
            mDefaultConsole.OnStdNotify += target;
      }

      static public bool testValidConnection()
      {
         if (mXboxManager == null)            return false;

         try
         {
            mDefaultConsole = mXboxManager.OpenConsole(mXboxManager.DefaultConsole);
            if (mDefaultConsole == null) return false;

            uint connID = mDefaultConsole.OpenConnection(mCommandStringHandle);
            mDefaultConsole.CloseConnection(connID);
         }
         catch
         {
            return false;
         }

         return true;

      }
      static private XboxManagerClass mXboxManager = null;
      static public XboxConsole mDefaultConsole = null;
      static private string mCommandStringHandle = @"XCMD";

      static private uint mCommandConnectionID = 0;

      static private eErrorType errorCatch(Exception e)
      {
         string msg = e.Message;
         if (msg.Contains("0x82DA0101"))
         {
            return eErrorType.cConnectionLost;
         }
         else if (msg.Contains("0x82DA0100"))
         {
            return eErrorType.cCannotConnect;
         }
         else if (msg.Contains("0x82DA0007"))
         {
            return eErrorType.cInvalidCommand;
         }
         return 0;
      }
   }


   static public class PhxGameInterface
   {
      ///////////////////////////////////////////
      ///////////////////////////////////////////
      ///////////////////////////////////////////
      ///////////////////////////////////////////

      static void ensureXFS()
      {
         Process[] processes = Process.GetProcesses();
         for (int i = 0; i < processes.Length; i++)
         {
            if (processes[i].ProcessName == @"xfs")
               return;
         }

         //XFS.EXE not found, run it.
         System.Diagnostics.Process.Start(CoreGlobals.getWorkPaths().mGameDirectory + mXFSPath);

      }


      static private string getBestExe()
      {
         try 
         {
         
            string consoleExe = @"";
            IXboxFiles f = XenonInterface.mDefaultConsole.DirectoryFiles(mConsoleDir);

            string[] appNames = new string[] { @"xgameF.xex", @"xgameP.xex", @"xgameD.xex" };
            bool found = false;
            for (int i = 0; i < 3; i++)
            {
               foreach (IXboxFile t in f)
               {
                  if (Path.GetFileName(t.Name) == appNames[i])
                  {
                     consoleExe = appNames[i];
                     found = true;
                     break;
                  }

               }
               if (found)
                  break;
            }

            return consoleExe;
         }
         catch
         {
         }

         return "";
      }


      static public bool launchApp()
      {
         
            ensureXFS();//ensure XFS is running before we try to launch the app.

            string consoleExe = getBestExe();
            if (consoleExe == "")
            {
               CoreGlobals.getErrorManager().OnSimpleWarning("Error quickviewing to 360 : No valid EXE file was found on the default console!");
               return false;
            }

            try
            {
               string gameExe = mConsoleDir + "\\" + consoleExe;
               XenonInterface.mDefaultConsole.Reboot(gameExe, mConsoleDir, "", XboxRebootFlags.Title);
            }
            catch
            {
               CoreGlobals.getErrorManager().OnSimpleWarning("Error quickviewing to 360 : Could not reboot the box!!");
               return false;
            }

            //allow a few sleep cycles so the game can boot up
            System.Threading.Thread.Sleep(3000);
         
        

         return true;
      }

      static public void launchScenerio(string scen)
      {

         string resp;
         XenonInterface.sendCommand(@"loadScenario(""" + scen + @""")" , out resp);
      }
      static public void reloadScenario()
      {
         string resp;
         XenonInterface.sendCommand(@"reloadScenario(true)", out resp);
      }

      static public bool testQuickviewRunning()
      {

         string resp;
         XenonInterface.sendCommand(@"giveScenarioName()", out resp);

         if (resp.ToLower().Contains("quickview"))
            return true;

         return false;
      }

      //phoenix app specific

      static private string mConsoleDir = @"devkit:\x\work";
      static private string mXFSPath = @"\tools\XFS\xfs.exe";
   }
}