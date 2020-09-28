#include "XenonInterface.h"

XenonInterface::eErrorType XenonInterface::init()
{
   if (mXboxManager == nullptr)
      mXboxManager = gcnew XboxManagerClass();

   mCurrentConsoleName = mXboxManager->DefaultConsole;               
   
   if (mXboxManager != nullptr && mCurrentConsole == nullptr)
      return changeCurrentConsole(mCurrentConsoleName);

   return eErrorType::cOK;
}

void XenonInterface::destroy()
{
   mCurrentConsole = nullptr;
   mXboxManager = nullptr;
}

XenonInterface::eErrorType XenonInterface::openConsole(String^ name)
{
   if (mXboxManager == nullptr)
      return eErrorType::cNotConnected;

   return changeCurrentConsole(name);
}

XenonInterface::eErrorType XenonInterface::createDirectory(String^ path)
{
   if (mCurrentConsole == nullptr)
      return eErrorType::cNotConnected;

   StringBuilder^ curPath = gcnew StringBuilder();

   int index = 0;
   while (index < path->Length)
   {
      wchar_t c = path[index];
      bool isSep = (c == '\\');

      if ((isSep) || (index == path->Length - 1))
      {
         if (index == path->Length - 1)
            curPath->Append(c);

         try
         {
            mCurrentConsole->MakeDirectory(curPath->ToString());
         }
         catch (Exception^)
         { 
         }
      }

      curPath->Append(c);

      index++;
   }

   return eErrorType::cOK;
}

XenonInterface::eErrorType XenonInterface::sendFile(String^ localName, String^ remoteName, bool createDir)
{
   return sendFile(mCurrentConsole, localName, remoteName, createDir);
}

XenonInterface::eErrorType XenonInterface::sendFile(String^ consoleName, String^ localName, String^ remoteName, bool createDir)
{
   XboxConsole^ console = nullptr;
   if (consoleName == mCurrentConsoleName)
      console = mCurrentConsole;
   else
   {
      try
      {
         console = mXboxManager->OpenConsole(consoleName);
      }
      catch (Exception^)
      { 
      }
   }
   if(console==nullptr)
      return eErrorType::cNotConnected;
   return sendFile(console, localName, remoteName, createDir);
}

XenonInterface::eErrorType XenonInterface::sendFile(XboxConsole^ console, String^ localName, String^ remoteName, bool createDir)
{
   if (console == nullptr)
      return eErrorType::cNotConnected;

   eErrorType res = eErrorType::cOK;
   
   try
   {
      if (createDir)
      {
         String^ path = remoteName;
         
         int i = path->LastIndexOf('\\');
         
         if (i >= 0)
            path = path->Substring(0, i);
         
         createDirectory(path);
      }

      console->SendFile(localName, remoteName);
   }
   catch (Exception^ e)
   {
      res = errorCatch(e);
   }
   
   return res;
}

void XenonInterface::reboot(String^ file, String^ mediaDir, String^ cmdLine, XboxRebootFlags flags)
{
   reboot(mCurrentConsole, file, mediaDir, cmdLine, flags);
}

void XenonInterface::reboot(String^ consoleName, String^ file, String^ mediaDir, String^ cmdLine, XboxRebootFlags flags)
{
   XboxConsole^ console = nullptr;
   
   if (consoleName == mCurrentConsoleName)
      console = mCurrentConsole;
   else
   {
      try
      {
         console = mXboxManager->OpenConsole(consoleName);
      }
      catch (Exception^)
      { 
      }
   }
   
   reboot(console, file, mediaDir, cmdLine, flags);
}

void XenonInterface::reboot(XboxConsole^ console, String^ file, String^ mediaDir, String^ cmdLine, XboxRebootFlags flags)
{
   if (console != nullptr)
   {
      try
      {
         console->Reboot(file, mediaDir, cmdLine, flags);
      }
      catch (Exception^)
      { 
      }
   }
}

XenonInterface::eErrorType XenonInterface::changeCurrentConsole(String^ consoleName)
{
   if (mCurrentConsole != nullptr)
   {
      if (mCurrentConsoleName == consoleName)
         return eErrorType::cOK;
   }
   
   try
   {
      mCurrentConsole = mXboxManager->OpenConsole(consoleName);
   }
   catch (Exception^ e)
   {
      mCurrentConsole = nullptr;
      return errorCatch(e);
   }

   mCurrentConsoleName = consoleName;
   mCurrentConsoleDebugIP = mCurrentConsole->IPAddress;
   mCurrentConsoleTitleIP = mCurrentConsole->IPAddressTitle;
   
   return eErrorType::cOK;
}    

XenonInterface::eErrorType XenonInterface::errorCatch(Exception^ e)
{
   try
   {
      System::Runtime::InteropServices::COMException^ ce = safe_cast<System::Runtime::InteropServices::COMException^>(e);
      
      if (ce)
      {
         switch ((unsigned)ce->ErrorCode)
         {
            case 0x82DA0101:
               return eErrorType::cConnectionLost;
            case 0x82DA0100:
            case 0x82DA0002:
               return eErrorType::cCannotConnect;
            case 0x82DA0007:
               return eErrorType::cInvalidCommand;
         }
      }            
   }
   catch (InvalidCastException^)
   {
   }
   
   return eErrorType::cFailed;
}
