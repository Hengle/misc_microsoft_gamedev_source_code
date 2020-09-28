//============================================================================
//
// File: bassert.cpp
//  
// Copyright (c) 1999-2006, Ensemble Studios
//
//============================================================================

#include "xcore.h"
#include "file\xboxFileUtils.h"
#include "bassertResource.h"
#include "fatalExit.h"

#ifdef XBOX
   #include "xexception\xdebugtext.h"
   #include "threading\eventDispatcher.h"
   #include <xbdm.h>
   #include "xdb\xdbManager.h"
#else
   #include "debughelp.h"
   #include "debugcallstack.h"
#endif

#ifdef USE_BUILD_INFO
#include "..\xgame\build_info.inc"
#endif

#ifdef CreateFileA
   #undef CreateFileA
#endif
#ifdef SetFilePointer
   #undef SetFilePointer
#endif
#ifdef CloseHandle   
   #undef CloseHandle
#endif   

static char cNone[] = "None.";

#ifndef XBOX
BOOL CALLBACK assertDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{ 
   switch (message) 
   { 
      case WM_COMMAND: 
      {
         switch(wParam)
         {
            // End the dialog box with proper return code if the user hit any of the buttons.
            case IDDEBUG:
            case IDABORT:
            case IDIGNORE:
            case IDIGNOREALL:
            case IDC_SEND:
               EndDialog(hwndDlg, wParam); 
         }
         return TRUE; 
      }

      case WM_INITDIALOG:
      {
         HWND wnd=GetDlgItem(hwndDlg, IDC_ERROR);
         if(wnd)
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getLargeFont(), TRUE);
         wnd=GetDlgItem(hwndDlg, IDC_FILELABEL);
         if(wnd)
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getBoldFont(), TRUE);
         wnd=GetDlgItem(hwndDlg, IDC_LINELABEL);
         if(wnd)
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getBoldFont(), TRUE);
         wnd=GetDlgItem(hwndDlg, IDC_EXPLABEL);
         if(wnd)
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getBoldFont(), TRUE);
         wnd=GetDlgItem(hwndDlg, IDC_MESSAGELABEL);
         if(wnd)
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getBoldFont(), TRUE);
         wnd=GetDlgItem(hwndDlg, IDC_CALLSTACKLABEL);
         if(wnd)
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getBoldFont(), TRUE);

         // Fill in the message string.
         wnd=GetDlgItem(hwndDlg, IDC_MSG);
         if(wnd)
         {
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getFixedFont(), TRUE);
            const char *txt=gAssertionSystem.getMessage();
            if(txt)
               SendMessageA(wnd, WM_SETTEXT, 0, (long)(void*)txt);
            else
               SendMessageA(wnd, WM_SETTEXT, 0, (long)(void*)cNone);
         }

         // Fill in the file string.
         wnd=GetDlgItem(hwndDlg, IDC_FILE);
         if(wnd)
         {
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getFixedFont(), TRUE);

            const char *txt=gAssertionSystem.getFile();
            if(txt)
               SendMessageA(wnd, WM_SETTEXT, 0, (long)(void*)txt);
            else
               SendMessageA(wnd, WM_SETTEXT, 0, (long)(void*)cNone);
         }


         // Fill in the line number.
         wnd=GetDlgItem(hwndDlg, IDC_LINE);
         if(wnd)
         {
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getFixedFont(), TRUE);

            char temp[256];
            StringCchPrintfA(temp, countof(temp), "%d", gAssertionSystem.getLine());
            temp[255] = 0;
            SendMessageA(wnd, WM_SETTEXT, 0, (long)(void*)temp);
         }

         // Fill in the expression.
         wnd=GetDlgItem(hwndDlg, IDC_EXP);
         if(wnd)
         {
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getFixedFont(), TRUE);

            const char *txt=gAssertionSystem.getExpression();
            if(txt)
               SendMessageA(wnd, WM_SETTEXT, 0, (long)(void*)txt);
            else
               SendMessageA(wnd, WM_SETTEXT, 0, (long)(void*)cNone);
         }

         // Fill in the callstack.
         wnd=GetDlgItem(hwndDlg, IDC_CALLSTACK);
         if(wnd)
         {
            SendMessageA(wnd, WM_SETFONT, (long)(void*)gAssertionSystem.getFixedFont(), TRUE);

            const BCHAR_T *txt=gAssertionSystem.getCallstackString();
            if(txt)
               SendMessage(wnd, WM_SETTEXT, 0, (long)(void*)txt);
            else
               SendMessage(wnd, WM_SETTEXT, 0, (long)(void*)cNone);
         }

         // If fatal, disable the ignore and ignore all buttons.
         wnd=GetDlgItem(hwndDlg, IDIGNORE);
         if(wnd)
         {
            if(gAssertionSystem.isFatal())
               EnableWindow(wnd, false);
            else
               EnableWindow(wnd, true);
         }
         wnd=GetDlgItem(hwndDlg, IDIGNOREALL);
         if(wnd)
         {
            if(gAssertionSystem.isFatal())
               EnableWindow(wnd, false);
            else
               EnableWindow(wnd, true);
         }
         
         // Force to top.
         RECT rect;
         GetWindowRect(hwndDlg, &rect);
         SetWindowPos(hwndDlg, HWND_TOPMOST, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,SWP_SHOWWINDOW);

         return(TRUE);
      }

   } 
   return FALSE; 
} 
#endif

BAssert::BAssert() :
   mFile(NULL), 
   mMessage(NULL), 
   mExpression(NULL), 
   mLine(-1), 
   mIgnoreAsserts(false),
   mShowDialog(true),
   mIgnoreListCount(0),
   mPreCallbackListCount(0),
   mPostCallbackListCount(0),
   mHasFailed(FALSE),
   mInitialized(true),
   mUseCacheDrive(false),
   mFirstWrite(true)
{
#ifdef XBOX
   mNumExceptionFilters = 0;
   Utils::ClearObj(mExceptionFilters);

   #if defined(BUILD_FINAL)
      BXboxFileUtils::initXboxCachePartition();

      testCacheDrive();
   #endif

#endif   
}

#ifdef XBOX
//==============================================================================
// BAssert::xbox_fail
//==============================================================================
long BAssert::platformFail(const char *expression, const char *msg, const char *file, long line, bool fatal, bool noreturn, bool triggeredByException)
{
   const DWORD lastWin32Error = GetLastError();
   
   const uint cBufSize = 1024;
   char       buf[cBufSize];
   SYSTEMTIME t;

   GetLocalTime(&t);

   BThreadIndex threadIndex = gEventDispatcher.getThreadIndex();
   
   if (!file)
      file = "NULL";

   if ((msg) && (expression))
      sprintf_s(buf, sizeof(buf), "%s(%i) at %s %s:\n%s (%s) (Win-32: 0x%X ThreadIndex: %i)\n", file, line, __DATE__, __TIME__, msg, expression, lastWin32Error, threadIndex);
   else if (msg)
      sprintf_s(buf, sizeof(buf), "%s(%i) at %s %s:\n%s (Win-32: 0x%X ThreadIndex: %i)\n", file, line, __DATE__, __TIME__, msg, lastWin32Error, threadIndex);
   else
      sprintf_s(buf, sizeof(buf), "%s(%i) at %s %s:\nExpression: %s (Win-32: 0x%X ThreadIndex: %i)\n", file, line, __DATE__, __TIME__, expression, lastWin32Error, threadIndex);
   
   // The stack trace will already been started from a specific IAR if this fail was triggered by our exception handler.
   if (!triggeredByException)
      gXDBManager.beginStackTrace();
      
   const int stackTraceLevels = gXDBManager.getStackTraceLevels();   
   
   const uint cFullBufSize = 8192;   
   BFixedString<cFullBufSize> fullBuf;

   fullBuf.formatAppend("== FAIL == %04d%02d%02dT%02d%02d%02d ==\n", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
#ifdef USE_BUILD_INFO
   fullBuf.append("DEPOT:"DEPOT_REVISION"|BUILD:"BUILD_NUMBER"|CHANGELIST:"CHANGELIST_NUMBER"\n");
#else
   fullBuf.append("NO_BUILD_INFO\n");
#endif

   #if !defined(BUILD_FINAL)
      DM_XBE xbeInfo;
      HRESULT hr = DmGetXbeInfo(NULL, &xbeInfo);
      if (hr == XBDM_NOERR)
      {
         fullBuf.formatAppend("XBE Launch Path: %s\n", xbeInfo.LaunchPath);
         fullBuf.formatAppend("XBE TimeStamp: %d\n", xbeInfo.TimeStamp);
         fullBuf.formatAppend("XBE CheckSum: %d\n", xbeInfo.CheckSum);
      }
      char dmXboxName[256];
      DWORD size = sizeof(dmXboxName);
      hr = DmGetXboxName(dmXboxName, &size);
      if (hr == XBDM_NOERR)
      {
         fullBuf.formatAppend("Xbox Name: %s\n", dmXboxName);
      }
   #endif

   CHAR gamerTag[XUSER_NAME_SIZE];
   for (DWORD i=0; i < 4; ++i)
   {
      if (XUserGetName(i, gamerTag, XUSER_NAME_SIZE) == ERROR_SUCCESS)
      {
         fullBuf.formatAppend("User (%d): %s\n", i, gamerTag);
      }
   }

   MEMORYSTATUS memStatus;
   GlobalMemoryStatus(&memStatus);
   fullBuf.formatAppend("Current Free: %u\n", memStatus.dwAvailPhys);

   fullBuf.append("== == ==\n");

   fullBuf.append(buf);
   if (stackTraceLevels > 0)
   {
      fullBuf.formatAppend("Stack trace levels: %u\n", stackTraceLevels);
      
      const BXDBFileReader::BLookupInfo& info = gXDBManager.getXDBLookupInfo();
      for (int i = 0; i < stackTraceLevels; i++)
      {
         if (!gXDBManager.walkStackTrace(i))
            fullBuf.formatAppend("0x%08X\n", gXDBManager.getStackTraceAddress(i));
         else
            fullBuf.formatAppend("%s(%i): %s (0x%08X)\n", info.mFilename.c_str(), info.mLine, info.mSymbol.c_str(), gXDBManager.getStackTraceAddress(i));   
      }
   }

#if 0   
   MEMORYSTATUS memStatus;
   GlobalMemoryStatus(&memStatus);
   fullBuf.formatAppend("Current Free: %u\n", memStatus.dwAvailPhys);
#endif   

   DWORD dwCreationDisposition = OPEN_ALWAYS;
   if (mFirstWrite)
   {
      dwCreationDisposition = CREATE_ALWAYS;
      mFirstWrite = false;
   }

#if defined(BUILD_FINAL)
   if (testCacheDrive())
   {
      HANDLE hFile = CreateFileA("cache:\\logs\\PhoenixFailReport.txt", GENERIC_WRITE, 0, 0, dwCreationDisposition, 0, 0);
      if (hFile != INVALID_HANDLE_VALUE)
      {
         SetFilePointer(hFile, 0, 0, FILE_END);
         DWORD bytesWritten;
         WriteFile(hFile, fullBuf.getPtr(), fullBuf.getLen(), &bytesWritten, 0);
         bytesWritten;
         CloseHandle(hFile);

         XFlushUtilityDrive();
      }
   }
#else
   // do not write data to the D drive in final builds
   HANDLE handle = CreateFileA("d:\\PhoenixFailReport.txt", GENERIC_WRITE, 0, 0, dwCreationDisposition, 0, 0);
   if (INVALID_HANDLE_VALUE != handle)
   {
      SetFilePointer(handle, 0, 0, FILE_END);
      
      DWORD bytesWritten;
      WriteFile(handle, fullBuf.getPtr(), fullBuf.getLen(), &bytesWritten, 0);
      bytesWritten;
      
      CloseHandle(handle);
   }
                        
   bool prompt = true;
   int y = 0;

   // Wait for GPU to finish blitting to the front buffer.
   Sleep(100);
   
   y = G_debug_text.render_cooked(0, 0, buf);
   if (y == 0)
      prompt = false;
   
   if ((prompt) && (stackTraceLevels > 0))
   {
      G_debug_text.render_raw(0, y, "Stack trace:");
      y++;
      
      const BXDBFileReader::BLookupInfo& info = gXDBManager.getXDBLookupInfo();

      for (int i = 0; i < stackTraceLevels; i++)
      {
         BFixedString256 str;
         if (!gXDBManager.walkStackTrace(i))
            str.format("0x%08X\n", gXDBManager.getStackTraceAddress(i));
         else
            str.format("%s(%i): %s (0x%08X)", info.mFilename.c_str(), info.mLine, info.mSymbol.c_str(), gXDBManager.getStackTraceAddress(i));
         G_debug_text.render_raw(0, y, str.c_str());
         y++;
      }
      
      y++;
   }      
   
   OutputDebugStringA(buf);
      
   if (DmIsDebuggerPresent())
   {
      // SINGLE STEP HERE ONCE BEFORE STOPPING, OTHERWISE THE DEVKIT WILL HARD REBOOT!
      breakpoint;
   }
#endif      

   // Call the pre-callback functions.
   for(long i=0; i<mPreCallbackListCount; i++)
      mPreCallbackList[i].mFunction( expression, msg, file, line, fatal, noreturn, NULL, mPreCallbackList[i].mParam1, mPreCallbackList[i].mParam2, fullBuf.getPtr() );

#ifndef BUILD_FINAL                  
   if ((noreturn) || (prompt))
   {            
      // First wait for buttons to be released
      for ( ; ; )
      {
         XINPUT_STATE state;
         DWORD res = XInputGetState(0, &state);
         if (ERROR_SUCCESS == res)
         {
            if ( ((state.Gamepad.wButtons & XINPUT_GAMEPAD_A) == 0) && 
               ((state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) == 0) &&
               ((state.Gamepad.wButtons & XINPUT_GAMEPAD_START) == 0) )
               break;              
         }
         Sleep(16);
      }
            
      if (noreturn)
         G_debug_text.render_cooked(0, y+4, "Execution Cannot Continue - Press Start to Reboot");
      else
      {
#ifdef BUILD_CHECKED
         G_debug_text.render_cooked(0, y+4, "Press A to Continue, B to Break, or Y to Ignore All");
#else      
         G_debug_text.render_cooked(0, y+4, "Press A to Continue or Y to Ignore All");
#endif         
      }
      
      Sleep(500);
      
      // rg [7/18/05] - Set shouldIgnore to true to ignore this assertion.
      static bool shouldIgnore;
      
      if (!shouldIgnore)
      {
         // Now wait for a button 
         for ( ; ; )
         {
            XINPUT_STATE state;
            DWORD res = XInputGetState(0, &state);
            if (ERROR_SUCCESS == res)
            {
               const bool buttonA = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
               const bool buttonB = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
               const bool buttonY = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
               const bool buttonStart = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
               buttonB;
               
               if (noreturn)
               {
                  if (buttonStart)
                  {
#ifndef BUILD_FINAL                  

                     DmReboot(DMBOOT_TITLE);
#else
                     break;      
#endif                  
                  }
               }
               else
               {
                  if (buttonA)
                     break;
#ifdef BUILD_CHECKED
                  else if (buttonB)
                  {
                     __asm twi 31, r0, 22
                  }
#endif                     
                  else if (buttonY)
                  {
                     shouldIgnore = true;
                     break;              
                  }
               }
            }
            Sleep(16);
         }
      }      
               
      if (shouldIgnore)
      {
         shouldIgnore = false;

#ifdef ENABLE_BASSERT_NORETURN         
         if (mAssertData.mpIgnoreFlag)
            *mAssertData.mpIgnoreFlag = true;
#endif            

         // Add to ignore list.
         if(mIgnoreListCount < cIgnoreListSize)
         {
            mIgnoreList[mIgnoreListCount].mLine=line;
            StringCchCopyNExA(mIgnoreList[mIgnoreListCount].mFile, sizeof(mIgnoreList[mIgnoreListCount].mFile), file, sizeof(mIgnoreList[mIgnoreListCount].mFile), NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
            mIgnoreList[mIgnoreListCount].mFile[sizeof(mIgnoreList[mIgnoreListCount].mFile) - 1]=0;
            mIgnoreListCount++;
         }
      }
   } 
#endif   

   // Call the post-callback functions.
   for(long i=0; i<mPostCallbackListCount; i++)
      mPostCallbackList[i].mFunction( expression, msg, file, line, fatal, noreturn, NULL, mPostCallbackList[i].mParam1, mPostCallbackList[i].mParam2, fullBuf.getPtr() );
   
   return (0);   
}

#else // !XBOX

//==============================================================================
// BAssert::win32_fail
//==============================================================================
long BAssert::platformFail(const char *expression, const char *msg, const char *file, long line, bool fatal, bool noreturn, bool triggeredByException)
{
   noreturn;
   triggeredByException;
      
   // Get a callstack if possible, skipping this function since we're interested in the caller on up.
   BDebugCallstack callstack;
   callstack;

   // jce [3/17/2003] -- Don't bother with this under the debugger since sometimes it's slow parsing through
   // the huge pdb files.
   if(!IsDebuggerPresent())
      gDebugHelp.stackWalk(callstack, 1);

   // HACK HACK HACK
   mCallstackString[0]=0;
   long num=callstack.getCount();
   const BDebugCallStackEntry *entries = callstack.getEntries();
   if(num<1 || !entries)
   {
      StringCchCopy(mCallstackString, countof(mCallstackString), B("Stack not available."));
   }
   else
   {
      long remainingChars=cCallstackStringSize;
      BCHAR_T *currPos=mCallstackString;
      BCHAR_T temp[1024];

      for(long i=0; i<callstack.getCount(); i++)
      {
         StringCchPrintf(temp, countof(temp), B("%s + 0x%x  -- %s(%d) -- %s\r\n"), BString(entries[i].mFunctionName).getPtr(), entries[i].mFunctionOffset, BString(entries[i].mFile).getPtr(), entries[i].mLine, BString(entries[i].mModule).getPtr());
         temp[1023]=0;
         StringCchCopyNEx(currPos, remainingChars, temp, remainingChars, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);

         long len=bcslen(temp);
         remainingChars-=len;
         currPos+=len;
      }
      mCallstackString[cCallstackStringSize-1]=0;
   }
         
   // Call the pre-callback functions.
   for(long i=0; i<mPreCallbackListCount; i++)
      mPreCallbackList[i].mFunction(expression, msg, file, line, fatal, noreturn, &callstack, mPreCallbackList[i].mParam1, mPreCallbackList[i].mParam2, NULL);

   // Fill in parameters.
   mExpression=expression;
   mMessage=msg;
   mFile=file;
   mLine=line;
   mFatal=fatal;

   // Create some fonts.
   mLargeFont=CreateFont(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, B("Arial"));
   mBoldFont=CreateFont(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, B("Arial"));
   mFixedFont=CreateFont(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, B("Courier New"));

   // Pop up dialog box.
   if(mShowDialog)
   {
      DWORD result=DialogBox(NULL, MAKEINTRESOURCE(IDD_BASSERT), NULL, assertDialogProc);

      if(mBoldFont)
         DeleteObject(mBoldFont);
      if(mFixedFont)
         DeleteObject(mFixedFont);

      if (FAILED(result))
      {
         __asm int 3;
      }

      // Do stuff based on the button pressed.  We do this here because it makes for a cleaner callstack when you
      // hit debug as compared with doing inside assertDialogProc.
      switch(result)
      {
      case IDDEBUG:
         // Throw a breakpoint.
         __asm int 3;
         break;

      case IDABORT:
         // End app.
         doFatalExit(0);
         break;

      case IDIGNORE:
         // Do nothing.
         break;

      case IDIGNOREALL:
         {
            // Add to ignore list.
            if(mIgnoreListCount < cIgnoreListSize)
            {
               mIgnoreList[mIgnoreListCount].mLine=line;
               StringCchCopyNExA(mIgnoreList[mIgnoreListCount].mFile, sizeof(mIgnoreList[mIgnoreListCount].mFile), file, sizeof(mIgnoreList[mIgnoreListCount].mFile), NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
               mIgnoreList[mIgnoreListCount].mFile[sizeof(mIgnoreList[mIgnoreListCount].mFile) - 1]=0;
               mIgnoreListCount++;
            }
            break;
         }

      case IDC_SEND:
         sendReport(expression, msg, file, line, fatal, callstack);
         break;
      };
   }

   // Call the post-callback functions.
   for(long i=0; i<mPostCallbackListCount; i++)
      mPostCallbackList[i].mFunction(expression, msg, file, line, fatal, noreturn, &callstack, mPostCallbackList[i].mParam1, mPostCallbackList[i].mParam2, NULL);
   
   return (0);
}
#endif // XBOX

//==============================================================================
// BAssert::fail
//==============================================================================
long BAssert::fail(const char *expression, const char *msg, const char *file, long line, bool fatal, bool triggeredByException)
{
   if (!mInitialized)
   {
#ifndef BUILD_FINAL
#ifdef XBOX   
      if (DmIsDebuggerPresent())
#else
      if (IsDebuggerPresent())
#endif
#endif
      {
         breakpoint;
      }

      for ( ; ; )
      {
         Sleep(1000);
      }
   }
   
   BScopedCriticalSection lock(mCriticalSection);
      
   static bool reEntered;

   if (reEntered)
   {
      trace("BAssert::fail: Reentrant fail() detected!");

#ifndef BUILD_FINAL
#ifdef XBOX
      Sleep(100);
      G_debug_text.render_cooked(0, 0, "REENTRANT BAssert::fail()!");
#endif
#endif

      for ( ; ; )
      {
         Sleep(16);
      }
   }
      
   reEntered = true;
   
   // If the assertion is not being allowed, bail out now.
   if(!isAssertionOk(file, line))
   {
      reEntered = false;
                  
      return (0);
   }
         
   InterlockedExchange(&mHasFailed, TRUE);
   
   trace("BAssert::fail: %s %s %s %i %u", expression, msg, file, line, fatal);
      
#ifdef XBOX
   long result = platformFail(expression, msg, file, line, fatal, false, triggeredByException);
#else
   long result = platformFail(expression, msg, file, line, fatal, false, triggeredByException);
#endif

   reEntered = false;
   
   InterlockedExchange(&mHasFailed, FALSE);
   
   return result;
}

//==============================================================================
// BAssert::failNoReturn
//==============================================================================
void BAssert::failNoReturn(const char *expression, const char *msg, const char *file, long line, bool fatal, bool triggeredByException)
{
   trace("BAssert::failNoReturn: %s %s %s %i %u %u", expression, msg, file, line, fatal, triggeredByException);

   if (!mInitialized)
   {
#ifndef BUILD_FINAL
#ifdef XBOX   
      if (DmIsDebuggerPresent())
#else
      if (IsDebuggerPresent())
#endif
#endif
      {
         breakpoint;
      }
      
      for ( ; ; )
      {
         Sleep(1000);
      }
   }
   
   BScopedCriticalSection lock(mCriticalSection);
   
   InterlockedExchange(&mHasFailed, TRUE);

#ifdef XBOX
   platformFail(expression, msg, file, line, fatal, true, triggeredByException);
#else
   platformFail(expression, msg, file, line, fatal, true, triggeredByException);
#endif
   
   for ( ; ; )
   {
      Sleep(1000);
   }
}

//==============================================================================
// BAssert::sendReport
//==============================================================================
void BAssert::sendReport(const char * /*expression*/, const char * /*msg*/, const char * /*file*/, long /*line*/, bool /*fatal*/, BDebugCallstack &callstack)
{
   if (!mInitialized)
      return;
      
#ifndef XBOX
   callstack;
#endif   
  
   BScopedCriticalSection lock(mCriticalSection);
   
   //-- BTK we need to figure out how to get XBOX watson to work on Xenon
   // rg [12/10/05] - Do this via a callback or something, I don't want watson stuff in xcore
#if 0
#ifndef XBOX 

   // Build an assertion file.

   // We want to give the context of the asserting function to watson so it reports it properly.
   // If we failed to get a stack, it's ok... we'll pass NULL and watson will get the stack.  It will
   // just (less conveniently) start inside the assert stuff instead of where the assertion triggered.
   const CONTEXT *context=NULL;
   if(callstack.getCount()>0)
      context=&(callstack.getEntries()[0].mContext);

   if(context)
      trace("eip=%d", context->Eip);

   // Send the data via watson
  
   uploadWatsonData(L"", L"Application Error Detected", L"Please choose 'Send Error Report' (again) so we can track this problem.", context);
#endif
#endif
}

//==============================================================================
// BAssert::testCacheDrive
//==============================================================================
bool BAssert::testCacheDrive()
{
#if defined(XBOX) && defined(BUILD_FINAL)
   if (BXboxFileUtils::isCacheInitialized())
   {
      if (CreateDirectory("cache:\\logs", NULL))
         mUseCacheDrive = true;
      else
      {
         DWORD dwError = GetLastError();
         if (dwError == ERROR_ALREADY_EXISTS)
            mUseCacheDrive = true;
      }
   }
   else
   {
      mUseCacheDrive = false;
   }
#endif
   return mUseCacheDrive;
}

//==============================================================================
// BAssert::isAssertionOk
//==============================================================================
bool BAssert::isAssertionOk(const char *file, long line)
{
   if ((!mInitialized) || (!file))
      return(true);
      
   // Ignoring ALL asserts?
   if(mIgnoreAsserts)
      return(false);

   // Check the ignore list for a match.
   for(long i=0; i<mIgnoreListCount; i++)
   {
      // If we don't match on line #, there is no match.
      if(line != mIgnoreList[i].mLine)
         continue;

      // If we match on filename, we match and the assertion is not valid to throw anymore.
      if(strcmp(file, mIgnoreList[i].mFile) == 0)
         return(false);
   }

   // No match, so feel free to bug user.
   return(true);
}


//==============================================================================
// BAssert::addPreCallback
//==============================================================================
bool BAssert::addPreCallback(BASSERT_CALLBACK_FUNC function, void *param1, void *param2)
{
   return(addCallback(mPreCallbackList, mPreCallbackListCount, function, param1, param2));
}


//==============================================================================
// BAssert::addPostCallback
//==============================================================================
bool BAssert::addPostCallback(BASSERT_CALLBACK_FUNC function, void *param1, void *param2)
{
   return(addCallback(mPostCallbackList, mPostCallbackListCount, function, param1, param2));
}


//==============================================================================
// BAssert::addCallback
//==============================================================================
bool BAssert::addCallback(BAssertCallbackEntry *list, long &count, BASSERT_CALLBACK_FUNC function, void *param1, void *param2)
{
   if (!mInitialized)
      return false;
      
   BScopedCriticalSection lock(mCriticalSection);
   
   // Don't bother adding a null function pointer.
   if(!function)
      return(false);

   // Is there room?
   if(count >= cCallbackListSize)
      return(false);

   // Fill in the info.
   list[count].mFunction=function;
   list[count].mParam1=param1;
   list[count].mParam2=param2;

   // Increment count
   count++;

   // Success.
   return(true);
}


//==============================================================================
// BAssert::clearCallbacks
//==============================================================================
void BAssert::clearCallbacks(void)
{
   if (!mInitialized)
      return;

   BScopedCriticalSection lock(mCriticalSection);
   
   clearPreCallbacks();
   clearPostCallbacks();
}


//==============================================================================
// BAssert::clearPreCallbacks
//==============================================================================
void BAssert::clearPreCallbacks(void)
{
   if (!mInitialized)
      return;
      
   BScopedCriticalSection lock(mCriticalSection);
   
   // Simple enough for now.
   mPreCallbackListCount = 0;
}


//==============================================================================
// BAssert::clearPostCallbacks
//==============================================================================
void BAssert::clearPostCallbacks(void)
{
   if (!mInitialized)
      return;
      
   BScopedCriticalSection lock(mCriticalSection);
   
   // Simple enough for now.
   mPostCallbackListCount = 0;
}

#ifdef XBOX
volatile BAssert::BExceptionAssertData BAssert::mAssertData;

//==============================================================================
// BAssert::XboxHandleAssertException
//==============================================================================
int BAssert::xboxHandleAssertException(_EXCEPTION_POINTERS* pExcept)
{
   {
      BScopedCriticalSection lock(mExceptionFilterCriticalSection);
      for (uint i = 0; i < mNumExceptionFilters; i++)
      {
         int status = (*mExceptionFilters[i].first)(pExcept, mExceptionFilters[i].second);
         if (status != EXCEPTION_CONTINUE_SEARCH)  
            return status;
      }
   }      
   
   char buf[256];
   sprintf_s(buf, sizeof(buf), "BAssert::xboxHandleAssertException: SEH exception at address 0x%X\n", pExcept->ContextRecord->Iar);
   OutputDebugStringA(buf);

   // Somebody writing to our magic address? Must be the break macro.
   if ((pExcept->ExceptionRecord->ExceptionCode != 0xC0000005) || (pExcept->ExceptionRecord->ExceptionInformation[1] != cXboxMagicBreakAddress))
      return EXCEPTION_CONTINUE_SEARCH;

   gXDBManager.beginStackTrace(pExcept);
   
   fail((const char*)mAssertData.mpExp, (const char*)mAssertData.mpMsg, (const char*)mAssertData.mpFile, mAssertData.mLine, false, true);
               
   // Skip over the break instruction
   pExcept->ContextRecord->Iar += 4;

   // Continue execution.
   return EXCEPTION_CONTINUE_EXECUTION;
}

bool BAssert::xboxAddExceptionFilterFunc(BExceptionFilterFuncPtr pFunc, DWORD data)
{
   BScopedCriticalSection lock(mExceptionFilterCriticalSection);
   
   if (mNumExceptionFilters == cMaxExceptionFilters)
      return false;
   mExceptionFilters[mNumExceptionFilters].first = pFunc;
   mExceptionFilters[mNumExceptionFilters].second = data;
   mNumExceptionFilters++;
   return true;
}

bool BAssert::xboxRemoveExceptionFilterFunc(BExceptionFilterFuncPtr pFunc, DWORD data)
{
   BScopedCriticalSection lock(mExceptionFilterCriticalSection);
   
   const std::pair<BExceptionFilterFuncPtr, DWORD> valueToFind(pFunc, data);
   
   for (uint i = 0; i < mNumExceptionFilters; i++)
   {
      if (mExceptionFilters[i] == valueToFind)
      {
         if (i != (mNumExceptionFilters - 1))
            mExceptionFilters[i] = mExceptionFilters[mNumExceptionFilters - 1];
         mNumExceptionFilters--;
         return true;
      }
   }
   
   return false;
}

#endif
