using System;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.Collections;
using System.Threading;
using XDevkit;

namespace xfs
{
   static public class XenonInterface
   {
      public enum eErrorType
      {
         cOK = 0,
         cFailed,
         cConnectionLost,
         cCannotConnect,
         cInvalidCommand,
         cNotConnected
      }

      static public XboxManagerClass mXboxManager = null;
      static public XboxConsole mCurrentConsole = null;
      static private string mCurrentConsoleName = "";
      static private ArrayList mDeferredCommands = new ArrayList();
      static private Thread mThread = null;
      static private AutoResetEvent mExitThread = new AutoResetEvent(false);
      static private int mRunningShellFlag = 0;
      
      static public bool getRunningShell()
      {
         return mRunningShellFlag > 2;
      }
      
      static public void resetRunningShellCheck()
      {
         System.Threading.Interlocked.Exchange(ref mRunningShellFlag, 0);  
      }
      
      static private void threadFunc()
      {
//         Debug.WriteLine("XenonInterface worker thread started");
         try
         {
            for ( ; ; )
            {
               WaitHandle[] handles = { mExitThread };
               int result = WaitHandle.WaitAny(handles, 2000, false);
               if (result == 0)
                  break;

               try
               {
                  XBOX_PROCESS_INFO processInfo = mCurrentConsole.RunningProcessInfo;
//                  Debug.WriteLine(processInfo.ProgramName);
                  
                  if (processInfo.ProgramName.Contains("xshell.xex"))
                  {
                     System.Threading.Interlocked.Increment(ref mRunningShellFlag);
                  }
                  else
                  {
                     System.Threading.Interlocked.Exchange(ref mRunningShellFlag, 0);
                  }
               }
               catch (Exception)
               {
//                  Debug.WriteLine("XenonInterface get RunningProcessInfo exception: " + e.ToString());
               }
            }
         }
         catch (Exception )
         {
//            Debug.WriteLine("XenonInterface worker thread exception: " + e.ToString());
         }
//         Debug.WriteLine("XenonInterface worker thread exiting");
      }
                        
      static private eErrorType changeCurrentConsole(String consoleName)
      {
         if (mThread != null)
         {
            mExitThread.Set();
            mThread.Join();
            mThread = null;
         }
         
         try
         {
             mCurrentConsole = mXboxManager.OpenConsole(consoleName);
         }
         catch (Exception e)
         {
            mCurrentConsole = null;
            return errorCatch(e);
         }
         
         mCurrentConsoleName = consoleName;

         mExitThread.Reset();
         mThread = new Thread(new ThreadStart(threadFunc));
         mThread.IsBackground = true;
         mThread.Start();
         
         return eErrorType.cOK;
      }    
      
      static public eErrorType init()
      {
         if (mXboxManager == null)
            mXboxManager = new XboxManagerClass();

         mCurrentConsoleName = mXboxManager.DefaultConsole;               
         
         if (mXboxManager != null && mCurrentConsole == null)
            return changeCurrentConsole(mCurrentConsoleName);

         return eErrorType.cOK;
      }

      static public void destroy()
      {
         if (mThread != null)
         {
            mExitThread.Set();
            mThread.Join();
            mThread = null;
         }
           
         mCurrentConsole = null;
         mXboxManager = null;
      }
            
      static public eErrorType openConsole(string name)
      {
         if (mXboxManager == null)
            return eErrorType.cNotConnected;
         
         return changeCurrentConsole(name);
      }

      static public eErrorType createDirectory(XboxConsole console, string path)
      {
         if (console == null)
            return eErrorType.cNotConnected;

         StringBuilder curPath = new StringBuilder();

         int index = 0;
         while (index < path.Length)
         {
            char c = path[index];
            bool isSep = (c == '\\');

            if ((isSep) || (index == path.Length - 1))
            {
               if (index == path.Length - 1)
                  curPath.Append(c);

               try
               {
                  console.MakeDirectory(curPath.ToString());
               }
               catch 
               { 
               }
            }
            
            curPath.Append(c);
            
            index++;
         }

         return eErrorType.cOK;
      }

      static public eErrorType sendFile(string localName, string remoteName, bool createDir, bool onlyIfNewer)
      {
         return sendFile(mCurrentConsole, localName, remoteName, createDir, onlyIfNewer);
      }

      static public eErrorType sendFile(string consoleName, string localName, string remoteName, bool createDir, bool onlyIfNewer)
      {
         XboxConsole console = null;
         if (consoleName == mCurrentConsoleName)
            console = mCurrentConsole;
         else
         {
            try
            {
               console = mXboxManager.OpenConsole(consoleName);
            }
            catch { }
         }
         if(console==null)
            return eErrorType.cNotConnected;
         return sendFile(console, localName, remoteName, createDir, onlyIfNewer);
      }

      static public eErrorType sendFile(XboxConsole console, string localName, string remoteName, bool createDir, bool onlyIfNewer)
      {
         if (console == null)
            return eErrorType.cNotConnected;

         eErrorType res = eErrorType.cOK;
         try
         {
            if (createDir)
            {
               string path = remoteName;
               int i = path.LastIndexOf('\\');
               if (i >= 0)
                  path = path.Substring(0, i);
               createDirectory(console, path);
            }

            bool gotLocalDateTime = false;
            DateTime localDateTime = DateTime.Now;
            try
            {
               localDateTime = File.GetLastWriteTime(localName);
               gotLocalDateTime = true;
            }
            catch { }

            if (onlyIfNewer && gotLocalDateTime)
            {
               IXboxFile fileObject = null;
               try
               {
                  fileObject = console.GetFileObject(remoteName);
               }
               catch { }
               if (fileObject != null)
               {
                  DateTime remoteDateTime = (DateTime)fileObject.ChangeTime;
                  if (localDateTime <= remoteDateTime)
                     return res;
               }
            }

            console.SendFile(localName, remoteName);
         }
         catch (Exception e)
         {
            res = errorCatch(e);
         }
         return res;
      }
      
      static public void captureScreenShot(string filename)
      {
         if (mCurrentConsole != null)
         {
            try
            {
               mCurrentConsole.ScreenShot(filename);
            } 
            catch { }
         }
      }

      static public void reboot(string file,string mediaDir, string cmdLine, XboxRebootFlags flags)
      {
         reboot(mCurrentConsole, file, mediaDir, cmdLine, flags);
      }

      static public void reboot(string consoleName, string file, string mediaDir, string cmdLine, XboxRebootFlags flags)
      {
         XboxConsole console = null;
         if (consoleName == mCurrentConsoleName)
            console = mCurrentConsole;
         else
         {
            try
            {
               console = mXboxManager.OpenConsole(consoleName);
            }
            catch { }
         }
         reboot(console, file, mediaDir, cmdLine, flags);
      }

      static public void reboot(XboxConsole console, string file, string mediaDir, string cmdLine, XboxRebootFlags flags)
      {
         if (console != null)
         {
            try
            {
               console.Reboot(file, mediaDir, cmdLine, flags);
            }
            catch { }
         }
      }
      
      static private eErrorType errorCatch(Exception e)
      {
         if (e is System.Runtime.InteropServices.COMException)
         {
            switch (((System.Runtime.InteropServices.COMException)e).ErrorCode)
            {
               case unchecked((int)0x82DA0101):
                  return eErrorType.cConnectionLost;
               case unchecked((int)0x82DA0100):
               case unchecked((int)0x82DA0002):
                  return eErrorType.cCannotConnect;
               case unchecked((int)0x82DA0007):
                  return eErrorType.cInvalidCommand;
            }
         }

         return eErrorType.cFailed;
      }
   }
   
   static public class PhxGameInterface
   {
      static public void launchScenerio(string scen)
      {
         // This is called from a worker thread.
         DebugConnectionManager.sendCommand(@"loadScenario(""" + scen + @""")");
      }

      static public void reloadFile(string filename)
      {
         // This is called from a worker thread.
         DebugConnectionManager.sendCommand(@"reloadFile(""" + filename + @""")");
      }

      static private string mConsoleDir = @"devkit:\x\work";
      static private string mXFSPath = @"\tools\XFS\xfs.exe";
   }
}