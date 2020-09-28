using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Threading;

namespace xfs
{
   public static class Program
   {
      static public MainForm mMainForm = null;
      static public InterProcessComm.InterProcessComm.IChannelManager mPipeManager = null;
      
      static private Mutex mXFSMutex;
      
      /// <summary>
      /// The main entry point for the application.
      /// </summary>
      [STAThread]
      public static void Main()
      {
         //try
         {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            mMainForm = new MainForm();
            
            if (!Server.setup(mMainForm.mDefaultIP))
               return;

            if (!DebugConnectionManager.setup())
               return;
               
            mMainForm.initData();
            mMainForm.Show();

            mPipeManager = new PipeManager();
            mPipeManager.Initialize();

            Server.start();
            DebugConnectionManager.start();

            try
            {
               mXFSMutex = new Mutex(true, "XFSMutex");
            }
            catch
            {
               return;
            }
                        
            Application.Run(mMainForm);

            mMainForm.saveCommandHistory();

                                    
            Server.shutdown();
            DebugConnectionManager.shutdown();
            XenonInterface.destroy();

            mPipeManager.Stop();
         }
         //catch { }
      }
   }
}