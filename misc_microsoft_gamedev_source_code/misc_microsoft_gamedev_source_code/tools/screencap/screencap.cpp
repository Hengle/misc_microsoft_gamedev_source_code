// screencap.cpp : main project file.

#include "stdafx.h"
#include "Form1.h"

using namespace screencap;

[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
   try
   {
      args;
      
	   // Enabling Windows XP visual effects before any controls are created
	   Application::EnableVisualStyles();
	   Application::SetCompatibleTextRenderingDefault(false); 
   				
	   Form1^ form1 = gcnew Form1;

	   // Create the main window and run it
	   Application::Run(form1);
   }
   finally
   {
      // rg [5/28/07] - Huge hack, because VS.Net 2005 shutdown sequence crashes during doexit().
      ExitProcess(0);
   }	   
	
	return 0;
}
