using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Text;

namespace RemoteMemory
{



   static class Program
   {
      /// <summary>
      /// The main entry point for the application.
      /// </summary>
      [STAThread]
      static void Main()
      {



         Application.EnableVisualStyles();
         Application.SetCompatibleTextRenderingDefault(false);

         GlobalSettings.load();
         MainForm frm = new MainForm();

         AllocLogStream.addListener(frm);

        

         Application.Run(frm);
         
      }
   }
}