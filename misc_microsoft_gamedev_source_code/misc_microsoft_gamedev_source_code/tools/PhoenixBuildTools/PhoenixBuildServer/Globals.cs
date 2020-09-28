using System;
using System.Collections.Generic;
using System.Text;

namespace PhoenixBuildServer
{
   class Globals
   {
      static private Config mConfig = null;
      static private MainForm mMainForm = null;

      static private string mServerIsIdleMessage = "Waiting for a job...";
      static private string mServerIsBusyMessage = "Running job #";

      static public Config Config
      {
         get { return mConfig; }
      }

      static public MainForm MainForm
      {
         get { return mMainForm; }
      }

      static public string ServerIsIdleMessage
      {
         get { return mServerIsIdleMessage; }
      }

      static public string ServerIsBusyMessage
      {
         get { return mServerIsBusyMessage; }
      }

      #region Public Functions

      static public bool init(string configFileName, MainForm mainform)
      {
         bool success;

         mMainForm = mainform;

         // Init config
         //
         mConfig = new Config();
         success = mConfig.init(configFileName);

         if (!success)
         {
            return (success);
         }

         return (true);
      }

      #endregion
   }
}
