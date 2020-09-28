using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace PublishIt
{
	static class Program
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main()
		{
          Application.ThreadException += new System.Threading.ThreadExceptionEventHandler(Application_ThreadException);
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			Application.Run(new Form1());
		}

       static void Application_ThreadException(object sender, System.Threading.ThreadExceptionEventArgs e)
       {
           MessageBox.Show(e.Exception.ToString());
       }
	}
}