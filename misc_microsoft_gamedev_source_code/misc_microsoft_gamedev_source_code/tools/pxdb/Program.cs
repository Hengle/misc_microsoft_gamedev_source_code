using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace pxdb
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

            Database.setup();

            Form mainForm = new MainForm();
            mainForm.Show();

            Application.Run(mainForm);        
        }
    }
}