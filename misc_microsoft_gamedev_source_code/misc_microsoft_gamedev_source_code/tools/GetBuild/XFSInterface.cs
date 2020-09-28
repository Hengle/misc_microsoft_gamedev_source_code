using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Threading;
using InterProcessComm.InterProcessComm;
using InterProcessComm.NamedPipes;

namespace GetBuild
{
   static public class XFSInterface
   {
      static string mLoadScenarioCMD   = "loadScenario";
      static string mLoadVisualCMD = "loadVisual";
      static string mLaunchGameCMD     = "launchGame";
      static string mGenerateFLSCMD =   "saveSHFillAt";
      static string mXFSPath           = @"\tools\XFS\xfs.exe";
      static string mXFSPipeName       = "XFSCommPipe";

      static public string connectSendMsg(string msg)
      {
         string retMsg = "";
         IInterProcessConnection clientConnection = null;
         try
         {
            clientConnection = new ClientPipeConnection(mXFSPipeName, ".");
            clientConnection.Connect();
            clientConnection.Write(msg);
            retMsg=clientConnection.Read() + "\n";
            clientConnection.Close();
         }
         catch (Exception ex)
         {
            clientConnection.Dispose();
            throw (ex);
         }
         return retMsg;
      }
      
      static bool ensureXFS()
      {
         Mutex mutex = null;
         try
         {
            mutex = System.Threading.Mutex.OpenExisting("XFSMutex");
         }
         catch
         {
         }
         
         if (mutex == null)
         {
            try
            {
               System.Diagnostics.Process.Start(Form1.mWorkPath + mXFSPath);
            }
            catch
            {
               return false;
            }
            
            const uint cTimesToSleep = 20*4;
            
            uint i;
            for (i = 0; i < cTimesToSleep; i++)
            {
               try
               {
                  mutex = System.Threading.Mutex.OpenExisting("XFSMutex");
               }
               catch
               {
               }
               if (mutex != null)
                  break;
               
               System.Threading.Thread.Sleep(250);
            }
            
            if (i == cTimesToSleep)
               return false;
         }
         
         return true;
      }
      
      static public bool launchApp()
      {
         return ensureXFS();
      }

      static public void launchScenario(string scn)
      {
         connectSendMsg(mLoadScenarioCMD + " " + '\"' + scn + '\"');
      }
      
      static public void launchGame()
      {
         connectSendMsg(mLaunchGameCMD);
      }

      static public void launchVisual(string unitName)
      {
         connectSendMsg(mLoadVisualCMD + " " + '\"' + unitName + '\"');
      }

      static public void generateFLS(float x, float y, float z, string saveTo)
      {
         //saveSHFillAt(0,200,0,"C:\\depot\\phoenix\\xbox\\work\\art\\lightsets\\e3_intro_start.fls")

         //CLM OMFG this is stupid. 
         // Somewhere between our IPC and the Xenon debug channels, all "\\" are being stripped to "\"
         // To combat this, turn all "\\" into "\\\\"
         string newPath = "";
         char[] delim = new char[] { '\\' };
         string[] tokens = saveTo.Split(delim);

         newPath += tokens[0];
         for (int i = 1; i < tokens.Length;i++ )
         {
            newPath += @"\\" + tokens[i];
         }
         //CLM the "!" tells XFS not to do crazy parsing on this, and instead just toss this string directly to the 360
         string cmd = "!" + mGenerateFLSCMD + "(" + x + "," + y + "," + z + "," + '\"' + newPath + "\")";
         int cmdLen = cmd.Length;
         connectSendMsg(cmd);
      }

      static public void pauseGame()
      {
         connectSendMsg("pauseGame");
      }
   }
}