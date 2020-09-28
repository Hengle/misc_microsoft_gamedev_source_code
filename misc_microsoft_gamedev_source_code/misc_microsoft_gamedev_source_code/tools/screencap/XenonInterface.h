#pragma once

using namespace System;
using namespace System::IO;
using namespace System::Text;
using namespace System::Windows::Forms;
using namespace System::Diagnostics;
using namespace System::Collections;
using namespace XDevkit;

public ref class XenonInterface
{
public:
   enum struct eErrorType : int
   {
      cOK               = 0,
      cFailed           = -100,
      cConnectionLost,
      cCannotConnect,
      cInvalidCommand,
      cNotConnected
   };

   static public XboxManagerClass^   mXboxManager = nullptr;
   static public XboxConsole^        mCurrentConsole = nullptr;
   static public String^             mCurrentConsoleName = ""; 
   static public UInt32              mCurrentConsoleDebugIP = 0; 
   static public UInt32              mCurrentConsoleTitleIP = 0; 
   
   static public eErrorType init();
   static public void destroy();

   static public eErrorType openConsole(String^ name);
   static public eErrorType createDirectory(String^ path);

   static public eErrorType sendFile(String^ localName, String^ remoteName, bool createDir);
   static public eErrorType sendFile(String^ consoleName, String^ localName, String^ remoteName, bool createDir);
   static public eErrorType sendFile(XboxConsole^ console, String^ localName, String^ remoteName, bool createDir);

   static public void reboot(String^ file, String^ mediaDir, String^ cmdLine, XboxRebootFlags flags);
   static public void reboot(String^ consoleName, String^ file, String^ mediaDir, String^ cmdLine, XboxRebootFlags flags);
   static public void reboot(XboxConsole^ console, String^ file, String^ mediaDir, String^ cmdLine, XboxRebootFlags flags);

private:   
   
   static eErrorType changeCurrentConsole(String^ consoleName);

   static private eErrorType errorCatch(Exception^ e);
};
