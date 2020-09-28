using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace GenerateScenarioOpenFileLists
{
   static class Program
   {
      static Form1 mMainFrm = null;
      /// <summary>
      /// The main entry point for the application.
      /// </summary>
      [STAThread]
      static void Main()
      {
         Application.EnableVisualStyles();
         Application.SetCompatibleTextRenderingDefault(false);
         mMainFrm = new Form1();
         Application.Run(mMainFrm);
      }

      public static ThreadSafeMessageList mMessageList = new ThreadSafeMessageList();

      static public void addStatusStringToForm(string msg, object obj)
      {
         mMainFrm.addStatusString(msg);
         
      }
   }
}