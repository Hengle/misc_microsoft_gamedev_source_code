using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace PhoenixEditor
{
   static class Program
   {
      /// <summary>
      /// The main entry point for the application.
      /// </summary>
      [STAThread]
      static int Main(string[] args)
      {
         Program.mbExporterRunning = false;
         Program.mbBatchExporter = null;
         try
         {

            //This must be first to supress any popup exception in the batch exporter
            if (args.Length != 0 && args[0] == "exporter")
            {
               Program.mbExporterRunning = true;
            }

            Xceed.DockingWindows.Licenser.LicenseKey = "DWN10-JHEY0-BZ3XS-EWCA";
            Xceed.Zip.Licenser.LicenseKey = "ZIN23-BUSZZ-NND31-7WBA";
            Xceed.SmartUI.Licenser.LicenseKey = "SUN33-2W6LR-JY5NS-6KNA";
            Xceed.Chart.Licenser.LicenseKey = "CHT40-A4SC3-BG1XW-SKAA";

            Xceed.Grid.Licenser.LicenseKey = "GRD30-AW6N0-0GC0H-7KXA"; 



            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            
            Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);
            Application.ThreadException += new System.Threading.ThreadExceptionEventHandler(Application_ThreadException);            

            if (args.Length == 0)
            {
               Application.Run(new MainWindow());
            }

            else if (args[0] == "exporter")
            {
               bool bCommandLineOnly = false;
               BatchExporter be = new BatchExporter();
               Program.mbBatchExporter = be;
               bool status = be.mainInit(args, out bCommandLineOnly);
               if (bCommandLineOnly == true)
               {
                  return be.mStatusResult;
               }
               else
               {
                  Application.Run(be);
                  return be.mStatusResult;
               }
            } 
            else
            {
               try
               {

                  Console.ForegroundColor = ConsoleColor.Red;
                  Application.Run(new MainWindow(args));
               }
               catch
               {

               }
            }

         }
         catch(System.Exception ex)
         {
            if (System.Diagnostics.Debugger.IsAttached)
               throw ex;
            if (Program.mbExporterRunning == false)
            {
               MessageBox.Show("If the editor is crashing on start up please check out the install instructions here \\\\esfile\\Phoenix\\Tools\\editor\\Dependencies  \n\n\n" + ex.ToString());
            }
            else
            {
               if (Program.mbBatchExporter != null)
               {
                  Program.mbBatchExporter.OnException(ex);
               }
               else //just incase the exporter dies in the constructor.
               {
                  Console.WriteLine(ex.ToString());
               }
            }
         }
         return 0;
      }
      static bool mbExporterRunning;
      static BatchExporter mbBatchExporter;

      static void Application_ThreadException(object sender, System.Threading.ThreadExceptionEventArgs e)
      {
         if (Program.mbExporterRunning == false)
         {
            EditorCore.CoreGlobals.getErrorManager().OnException(e.Exception);
         }
         else
         {
            Program.mbBatchExporter.OnException(e.Exception);
         }
      }

      [DllImport("kernel32", SetLastError = true)]
      static extern bool AttachConsole(int dwProcessId); 
   }
}
