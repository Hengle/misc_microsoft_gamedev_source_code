//==============================================================================
// exception.cpp
//
// Copyright (c) 2001-2002 Ensemble Studios
//
// Various bits of this code are adopted from MSDN
//==============================================================================

#include "xcore.h"
//#include <dbghelp.h>
#include "exception.h"

#ifndef XBOX
#pragma warning(disable: 4995)
#include <shlwapi.h>
#pragma warning(default: 4995)
#endif

#ifdef DEBUG
   #undef DEBUG
   #define REDODEBUG
#endif
#include "msodw.h"
#ifdef REDODEBUG
   #define DEBUG
#endif

#define EB(a) L##a

// jce [6/23/2005] -- FIXME BEFORE SHIP -- turning this to internal mode for now.
#undef BUILD_FINAL


#ifdef XBOX
static bool IsDebuggerPresent()
{
   //FIXME
   #ifdef FINAL_RELEASE
      return false;
   #else
      return true;
   #endif
}
#endif

//============================================================================
// 
//============================================================================
static LPTOP_LEVEL_EXCEPTION_FILTER gpPreviousExceptionFilter;


//============================================================================
//  PRIVATE GLOBALS
//============================================================================
static TRACE_FUNC                *sgpTraceFunc=NULL;
static MESSAGE_FUNC              *sgpMessageFunc=NULL;
static GET_STRING_FUNC           *sgpAppNameFunc=NULL;
static DISPLAY_WAIT_DLG_FUNC     *sgpExceptionWaitDlg=NULL;
static PROMPT_USER_INFO_FUNC     *sgpExceptionUserInfoDlg=NULL;
static PRE_POST_FUNC             *sgpPreFunc=NULL;
static PRE_POST_FUNC             *sgpPostFunc=NULL;
static HWND                      sghWnd=NULL;

static WCHAR gcLogDir[MAX_PATH];
static WCHAR gcExeDir[MAX_PATH];

//============================================================================
//  CALLBACK REGISTRATION
//============================================================================
void registerPreFunc(PRE_POST_FUNC *pFunc)
{
   sgpPreFunc = pFunc;
}

void registerPostFunc(PRE_POST_FUNC *pFunc)
{
   sgpPostFunc = pFunc;
}

//============================================================================
// 
//============================================================================
void registerTraceFunc(TRACE_FUNC* pFunc)
{
   sgpTraceFunc = pFunc;
}

//==============================================================================
//
//==============================================================================
void registerMessageFunc(MESSAGE_FUNC* pFunc)
{
   sgpMessageFunc = pFunc;
}

//==============================================================================
//
//==============================================================================
void registerAppNameFunc(GET_STRING_FUNC *pFunc)
{
   sgpAppNameFunc = pFunc;
}

//==============================================================================
// registerExceptionWaitDlg
//==============================================================================
void registerExceptionWaitDlg(DISPLAY_WAIT_DLG_FUNC *pFunc)
{
   sgpExceptionWaitDlg = pFunc;
}

//==============================================================================
// registerExceptionDescriptionDlg
//==============================================================================
void registerExceptionDescriptionDlg(PROMPT_USER_INFO_FUNC *pFunc)
{
   sgpExceptionUserInfoDlg = pFunc;
}

//==============================================================================
// 
//==============================================================================
static const long BUFFER_SIZE = 8192;
static WCHAR szBuffer[BUFFER_SIZE];

const WCHAR* decodeSystemError(DWORD dwerror, WCHAR* lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);

	long nBuf = bvsnwprintf(szBuffer, BUFFER_SIZE, lpszFormat, args);

   BASSERT(nBuf >= 0);          // no errors
   BASSERT(nBuf < BUFFER_SIZE); // it fit

   if(nBuf>=0)
   {
#ifndef XBOX
	   nBuf += FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						     dwerror,
						     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						     &szBuffer[nBuf], BUFFER_SIZE - nBuf, NULL );// Display the string.
#endif
      // append a \r\n if necessary
      if ((nBuf<BUFFER_SIZE-2) && szBuffer[nBuf-1] != EB('\n')) 
      {
         szBuffer[nBuf] = EB('\n');
         szBuffer[nBuf+1] = 0;
      }
      BASSERT((nBuf+1) < BUFFER_SIZE);

	   OutputDebugStringW(szBuffer);
   }

	va_end(args);

	return szBuffer;

} // decodeSystemError

//==============================================================================
// uploadWatsonData
//
// pFileList = list of | delimited files
//==============================================================================
void uploadWatsonData(const WCHAR* pFileList, const WCHAR* wzHeader, const WCHAR* wzItsOkay, const CONTEXT *optionalContext)
{
#ifdef XBOX
   //FIXME
   pFileList;
   wzHeader;
   wzItsOkay;
   optionalContext;
#else
	HANDLE              hFileMap = NULL;
	DWSharedMem*        pdwsm    = NULL;
	SECURITY_ATTRIBUTES sa;
	
	// we keep local copies of these in case another thread is trashing memory
	// it much more likely to trash the heap than our stack
	HANDLE hEventDone = NULL;          // event DW signals when done
	HANDLE hEventAlive = NULL;         // heartbeat event DW signals per EVENT_TIMEOUT
	HANDLE hMutex = NULL;              // to protect the signaling of EventDone  
	HANDLE hProc = NULL;
	WCHAR szCommandLine[MAX_PATH * 2];
	WCHAR szAppFolderPath[MAX_PATH];
	WCHAR szApplicationName[MAX_PATH];
   
	WCHAR* pch;
	DWORD dw;
	BOOL fDwRunning; 
	DWORD msoctdsResult;
   WCHAR buffer[256] = { 0 };
   long dwlen;
   long dwCodePage = 1033;
   STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	
   CONTEXT context;
   const CONTEXT *contextToUse;
   if(optionalContext)
      contextToUse=optionalContext;
   else
   {
      contextToUse=&context;
      memset(&context, 0, sizeof(context));
  
      // jce [11/20/2002] -- GetThreadContext can't ever work on GetCurrentThread().  It can only operate
      // on suspended threads.
      //GetThreadContext(GetCurrentThread(), &context);

      // Grab our context info ... instruction pointer/stack info.
      #define push_eip __asm _emit 0e8h __asm _emit 00h __asm _emit 00h __asm _emit 00h __asm _emit 00h
      DWORD saveEIP;
      DWORD saveEBP;
      DWORD saveESP;
      push_eip
      __asm pop eax
      __asm mov saveEIP, eax;
      __asm mov saveEBP, ebp;
      __asm mov saveESP, esp;
      context.Ebp=saveEBP;
      context.Eip=saveEIP;
      context.Esp=saveESP;
   }

	// fake up an exception record
   EXCEPTION_RECORD er;
   //er.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION; 
   er.ExceptionCode = EXCEPTION_BREAKPOINT; 
   er.ExceptionFlags = 0; 
   er.ExceptionRecord = 0;
   er.ExceptionAddress = (PVOID)contextToUse->Eip;
   er.NumberParameters = 0;
   er.ExceptionInformation[0] = 0;

   EXCEPTION_POINTERS ep;

   ep.ExceptionRecord = &er;
   ep.ContextRecord = (CONTEXT *)contextToUse;

   LPEXCEPTION_POINTERS pep = &ep;

   // Call the pre-watson callback (which might minimize the game, flush log files, etc.)
   if(sgpPreFunc)
      sgpPreFunc();

   // Kick off the "hang on a sec" app that runs dxdiag, etc.
   WCHAR watsonInfo[MAX_PATH];
   #ifdef BUILD_FINAL
   StringCchPrintfW(watsonInfo, countof(watsonInfo), EB("%swatsoninfo.exe"), gcExeDir);
   #else
   StringCchPrintfW(watsonInfo, countof(watsonInfo), EB("%swatsoninfointernal.exe"), gcExeDir);
   #endif
	memset(&si, 0, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW);
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));

   // SAT (6/13/05) - Changed watsonInfo to go in the first parameter instead of second to fix Prefix issue.
   BOOL processCreated=CreateProcessW(watsonInfo, NULL, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL, gcLogDir, &si, &pi);

   // Wait for it to end.
   if(processCreated)
   {
      WaitForSingleObject(pi.hProcess, INFINITE);

      // Close handles.
      CloseHandle(pi.hProcess);
	   CloseHandle(pi.hThread);
   }

	// create shared memory
	memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	
	hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, 
					  sizeof(DWSharedMem), NULL);
	if (hFileMap == NULL)
	{
#ifdef _DEBUG
		MessageBoxW(sghWnd, EB("Fail 1"), EB("failure"), MB_OK);
#endif
		goto LErrorCleanup;
	}
		
	pdwsm = (DWSharedMem *) MapViewOfFile(hFileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (pdwsm == NULL)
	{
#ifdef _DEBUG
		MessageBoxW(sghWnd, EB("Fail 2"), EB("failure"), MB_OK);
#endif
		goto LErrorCleanup;
	}

	memset(pdwsm, 0, sizeof(DWSharedMem));



   WCHAR cUserDataFile[MAX_PATH];
   StringCchPrintfW(cUserDataFile, countof(cUserDataFile), EB("%suserDataFile.txt"), gcLogDir);

   WCHAR cDxDiagFile[MAX_PATH];
   StringCchPrintfW(cDxDiagFile, countof(cDxDiagFile), EB("%sdxdiag.txt"), gcLogDir);

   // get file data
   pdwsm->wzAdditionalFile[0] = 0;

   StringCchCopyW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), cDxDiagFile);
   #ifndef BUILD_FINAL
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("|"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), cUserDataFile);
   #endif
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("|"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), gcLogDir);
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("logfile.txt"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("|"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), gcLogDir);
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("memlog.txt"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("|"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), gcLogDir);
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("commlog.txt"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("|"));

   // the Age3log.txt needs to be copied before being uploaded
   WCHAR cCopySource[MAX_PATH], cCopyDest[MAX_PATH];
   StringCchPrintfW(cCopySource, countof(cCopySource), EB("%sAge3Log.txt"), gcLogDir);
   StringCchPrintfW(cCopyDest, countof(cCopyDest), EB("%slogfile.txt"), gcLogDir);
   DeleteFileW(cCopyDest);
   CopyFileW(cCopySource, cCopyDest, FALSE);

   // same for mem log
   StringCchPrintfW(cCopySource, countof(cCopySource), EB("%sAge3mmLog.txt"), gcLogDir);
   StringCchPrintfW(cCopyDest, countof(cCopyDest), EB("%smemlog.txt"), gcLogDir);
   DeleteFileW(cCopyDest);
   CopyFileW(cCopySource, cCopyDest, FALSE);

   // comm log
   StringCchPrintfW(cCopySource, countof(cCopySource), EB("%sAge3commLog.txt"), gcLogDir);
   StringCchPrintfW(cCopyDest, countof(cCopyDest), EB("%scommlog.txt"), gcLogDir);
   DeleteFileW(cCopyDest);
   CopyFileW(cCopySource, cCopyDest, FALSE);


//   // convert to unicode
	bsnwprintf(pdwsm->wzAdditionalFile, sizeof(pdwsm->wzAdditionalFile) / sizeof(pdwsm->wzAdditionalFile[0]), EB("%s|%s"), pdwsm->wzAdditionalFile, pFileList);

   // setup exchange events and notifications

	hEventAlive = CreateEvent(&sa, FALSE, FALSE, NULL);
	hEventDone = CreateEvent(&sa, FALSE, FALSE, NULL);
	hMutex = CreateMutex(&sa, FALSE, NULL);

	if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), 
					GetCurrentProcess(), &hProc, PROCESS_ALL_ACCESS,
					TRUE, 0))
   {
#ifdef _DEBUG
      WCHAR szAssert[1024];
		bsnwprintf(szAssert, 1024, EB("Fail on DuplicateHandle (%d)"), GetLastError());
      szAssert[1023] = 0;
		MessageBoxW(sghWnd, szAssert, EB("failure"), MB_OK);
#endif
		goto LErrorCleanup;
	}

	if (hEventAlive == NULL || hEventDone == NULL || hMutex == NULL 
      || hProc == NULL)
	{
#ifdef _DEBUG
		MessageBoxW(sghWnd, EB("Fail 3"), EB("failure"), MB_OK);
#endif
		goto LErrorCleanup;
	}

	// setup interface structure. These are all based on stuff we have done above.
	pdwsm->hProc = hProc; // local reference, we use hProc for closing later
	pdwsm->pid = GetCurrentProcessId();
	pdwsm->tid = GetCurrentThreadId();
	pdwsm->hEventAlive = hEventAlive;
	pdwsm->hEventDone = hEventDone;
	pdwsm->hMutex = hMutex;
	pdwsm->dwSize = sizeof(DWSharedMem);
	pdwsm->pep = pep;
	pdwsm->eip = contextToUse->Eip; // dwAddress;

	// Here we choose to see a Restart checkbox. If you don't want it, just send msoctdsQuit.
	// You could send msoctdsQuit | msoctdsRecover if you want to show "Recover my work and restart ..." 
	// You don't need to set msoctdsDebug -- DW will look for a JIT and show it itself.

   pdwsm->bfmsoctdsOffer = msoctdsQuit;

	// Testcrash only wants to hear back if the user hit Quit (or implicitly, Debug).
	// Otherwise kill the process (in our case, since we offer Restart, DW will do that for us).
	pdwsm->bfmsoctdsLetRun = (DWORD) -1; // msoctdsQuit | msoctdsDebug;

	// don't set fDwOfficeApp if you're not one.
	// don't set fDwUsePrivacyHTA if you don't ship one
	pdwsm->bfDWBehaviorFlags = fDwReportChoice | fDwUseHKLM;

   StringCchCopyW(pdwsm->wzErrorText, countof(pdwsm->wzErrorText), wzItsOkay); 
   StringCchCopyW(pdwsm->wzHeader, countof(pdwsm->wzHeader), wzHeader); 

   // Get the application name from the callback if possible.
   if(sgpAppNameFunc)
	   StringCbCopyW(pdwsm->wzFormalAppName, DW_APPNAME_LENGTH*sizeof(*pdwsm->wzFormalAppName), sgpAppNameFunc());
   else
   {
      // If no callback was registered, just grab the current module name.
	   DWORD size=GetModuleFileNameW(0, pdwsm->wzFormalAppName, DW_APPNAME_LENGTH);
      if(size==DW_APPNAME_LENGTH)
         pdwsm->wzFormalAppName[DW_APPNAME_LENGTH-1]=0;
   }

	// update me!	
   StringCchCopyA(pdwsm->szRegSubPath, countof(pdwsm->szRegSubPath), "Microsoft\\PCHealth\\ErrorReporting\\DW");

	// don't update unless you install your own or something....
   StringCchCopyA(pdwsm->szPIDRegKey, countof(pdwsm->szPIDRegKey), "HKLM\\Software\\Microsoft\\Internet Explorer\\Registration\\DigitalProductID");

	// switch to watson.microsoft.com when you're ready to ship.
	// debug and retail versions of testcrash/dw can report to officewatson.
	// only retail versions of dw can report to watson.microsoft.com
   StringCchCopyA(pdwsm->szServer, countof(pdwsm->szServer), "watson.microsoft.com");

	// Here's a 9x-safe way to get your module file name.
	// You can use GetModuleFileNameW if you know you'll only run on NT flavors
   //-- Actually UnicoWS.lib is supposed to let us do this.
	GetModuleFileNameW(NULL, pdwsm->wzModuleFileName, DW_MAX_PATH);
//	MultiByteToWideChar(CP_ACP, 0, szModPath, -1, pdwsm->wzModuleFileName, DW_MAX_PATH);

	// Do not hardcode this, but rather use your vlcidUI value.
	// Also, do not send us 1033 universally and swap dlls underneath us to simplify your setup logic.  
	// We use this to determine which privacy policy to display, so it has to be right.

   dwlen = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILANGUAGE, buffer, 256);

   if (dwlen)
   {
      WCHAR *cp = 0;

      dwCodePage = wcstol(buffer, &cp, 16);
   }

   pdwsm->lcidUI = dwCodePage;

	// Figure out where you want to look for DW.  This implementation assumes it's next to the app.
	GetModuleFileNameW(NULL, szAppFolderPath, MAX_PATH);	
	pch = szAppFolderPath + wcslen(szAppFolderPath);
			
	// chomp exe name
	while (pch >= szAppFolderPath && *pch != EB('\\'))
		pch--;

	if (pch < szAppFolderPath)
   {
#ifdef _DEBUG
		MessageBoxW(sghWnd, EB("Fail 4: generating full path to DW.exe"), EB("failure"), MB_OK);
#endif
		goto LErrorCleanup;
	}

	*pch = EB('\0'); // prune


   #ifndef BUILD_FINAL
   // HI EVERYBODY!!  It's time to call DrNick.  Then we'll let dr watson go.
   StringCchPrintfW(szApplicationName, countof(szApplicationName), EB("%s\\drnick.exe"), szAppFolderPath);
   StringCchPrintfW(szCommandLine, countof(szCommandLine), EB(" %u"), (DWORD) hFileMap);
	processCreated = CreateProcessW(szApplicationName, szCommandLine, NULL, NULL, TRUE, 
				  CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL,
				  NULL, &si, &pi);

   // Try production dir.
   if(!processCreated)
   {
   	processCreated = CreateProcessW(EB("drnick.exe"), szCommandLine, NULL, NULL, TRUE, 
	      CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL,
			NULL, &si, &pi);
   }

   // Wait for it to end.
   if(processCreated)
   {
      WaitForSingleObject(pi.hProcess, INFINITE);

      // Close handles.
      CloseHandle(pi.hProcess);
	   CloseHandle(pi.hThread);
   }
   #endif


	// it's better security to set a full path for the app than to let the OS go looking for it.
   StringCchPrintfW(szApplicationName, countof(szApplicationName), EB("%s\\dw15.exe"), szAppFolderPath);
   StringCchPrintfW(szCommandLine, countof(szCommandLine), EB(" -x -s %u"), (DWORD) hFileMap);

	memset(&si, 0, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW);
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));

   // call watson!

	if (CreateProcessW(szApplicationName, szCommandLine, NULL, NULL, TRUE, 
				  CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL,
				  NULL, &si, &pi))
	{
		fDwRunning = TRUE;
		while (fDwRunning)
		{
			if (WaitForSingleObject(hEventAlive, DW_TIMEOUT_VALUE) == WAIT_OBJECT_0)
			{
				if (WaitForSingleObject(hEventDone, 1) == WAIT_OBJECT_0)
				{
					fDwRunning = FALSE;
				}
			   continue;
			}
				
			 // we timed-out waiting for DW to respond, try to quit
			dw = WaitForSingleObject(hMutex, DW_TIMEOUT_VALUE);
			if (dw == WAIT_TIMEOUT)
				fDwRunning = FALSE; // either DW's hung or crashed, we must carry on  
			else if (dw == WAIT_ABANDONED)
			{
				fDwRunning = FALSE;
				ReleaseMutex(hMutex);
			}
			else
			{
				// DW has not woken up?
				if (WaitForSingleObject(hEventAlive, 1) != WAIT_OBJECT_0)
					// tell DW we're through waiting for it's sorry self
				{
					SetEvent(hEventDone);
					fDwRunning = FALSE;
				}
				else
				{
					// are we done
					if (WaitForSingleObject(hEventDone, 1) == WAIT_OBJECT_0)
						fDwRunning = FALSE;
				}
				ReleaseMutex(hMutex);
			}
		}

		// no, clean up
		CloseHandle(hEventAlive);
		hEventAlive = NULL;
		CloseHandle(hEventDone);
		hEventDone = NULL;
		CloseHandle(hMutex);
		hMutex = NULL;
		CloseHandle(hProc);
		hProc = NULL;
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	} // end if CreateProcess succeeded
	
	msoctdsResult = pdwsm->msoctdsResult;

	UnmapViewOfFile(pdwsm);
	pdwsm = NULL;
	CloseHandle(hFileMap);
	hFileMap = NULL;

   // Call the post-watson callback (which might unminimize the game, etc.)
   if(sgpPostFunc)
      sgpPostFunc();

   // we use a local to let the file map etc. get cleaned up first.
   return;

LErrorCleanup:

   // Call the post-watson callback (which might unminimize the game, etc.)
   if(sgpPostFunc)
      sgpPostFunc();

   if (hEventAlive)
		CloseHandle(hEventAlive);
	if (hEventDone)
		CloseHandle(hEventDone);
	if (hMutex)
		CloseHandle(hMutex);
	if (hProc)
		CloseHandle(hProc);
	if (pdwsm)
		UnmapViewOfFile(pdwsm);
	if (hFileMap)
		CloseHandle(hFileMap);
#endif
} // uploadWatsonData


//==============================================================================
//
//==============================================================================
LONG WINAPI watsonExceptionHandler(LPEXCEPTION_POINTERS pep)
{
#ifdef XBOX
   //FIXME
   pep;
#else
	EXCEPTION_RECORD *per;
	HANDLE hFileMap = NULL;
	DWSharedMem* pdwsm = NULL;
	SECURITY_ATTRIBUTES  sa;
	
	// we keep local copies of these in case another thread is trashing memory
	// it much more likely to trash the heap than our stack
	HANDLE hEventDone = NULL;          // event DW signals when done
	HANDLE hEventAlive = NULL;         // heartbeat event DW signals per EVENT_TIMEOUT
	HANDLE hMutex = NULL;              // to protect the signaling of EventDone  
	HANDLE hProc = NULL;
	WCHAR szCommandLine[MAX_PATH * 2];
	WCHAR szAppFolderPath[MAX_PATH];
	WCHAR szApplicationName[MAX_PATH];
  
	WCHAR* pch;
	DWORD  dw;
	BOOL   fDwRunning; 
	DWORD  msoctdsResult;
   WCHAR  buffer[256] = { 0 };
   long   dwlen;
   long   dwCodePage = 1033;

   STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	
	// init
	per = pep->ExceptionRecord;

   if( EXCEPTION_BREAKPOINT==per->ExceptionCode || EXCEPTION_SINGLE_STEP==per->ExceptionCode )
		return EXCEPTION_CONTINUE_EXECUTION;

   // if available, ask the user what the hell happened

	// you can uncomment this to throw you into the debugger for testing purposes
	// this works in testcrash, but I've had problems with trying to do it in "real apps."
	//__asm{int 3};  


   // Kick off the "hang on a sec" app that runs dxdiag, etc.
   WCHAR watsonInfo[MAX_PATH];
   #ifdef BUILD_FINAL
   StringCchPrintfW(watsonInfo, countof(watsonInfo), EB("%swatsoninfo.exe"), gcExeDir);
   #else
   StringCchPrintfW(watsonInfo, countof(watsonInfo), EB("%swatsoninfointernal.exe"), gcExeDir);
   #endif
   memset(&si, 0, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW);
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));

   // SAT (6/13/05) - Changed watsonInfo to go in the first parameter instead of second to fix Prefix issue.
   BOOL processCreated=CreateProcessW(watsonInfo, NULL, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL, gcLogDir, &si, &pi);

   // Wait for it to end or just give up after 60 secs.
   if(processCreated)
   {
      WaitForSingleObject(pi.hProcess, 60000);

      // Close handles.
      CloseHandle(pi.hProcess);
	   CloseHandle(pi.hThread);
   }


	// create shared memory
	memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	
	hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, 
					  sizeof(DWSharedMem), NULL);
	if (hFileMap == NULL)
	{
#ifdef _DEBUG
		MessageBoxW(sghWnd, EB("Fail 1"), EB("failure"), MB_OK);
#endif
		goto LErrorCleanup;
	}
		
	pdwsm = (DWSharedMem *) MapViewOfFile(hFileMap, 
										  FILE_MAP_READ | FILE_MAP_WRITE,
										  0, 0, 0);
	if (pdwsm == NULL)
	{
#ifdef _DEBUG
		MessageBoxW(sghWnd, EB("Fail 2"), EB("failure"), MB_OK);
#endif
		goto LErrorCleanup;
	}

	memset(pdwsm, 0, sizeof(DWSharedMem));

   WCHAR cUserDataFile[MAX_PATH];
   StringCchPrintfW(cUserDataFile, countof(cUserDataFile), EB("%suserDataFile.txt"), gcLogDir);

   WCHAR cDxDiagFile[MAX_PATH];
   StringCchPrintfW(cDxDiagFile, countof(cDxDiagFile), EB("%sdxdiag.txt"), gcLogDir);

   // get file data
   pdwsm->wzAdditionalFile[0] = 0;

   StringCchCopyW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), cDxDiagFile);
#ifndef BUILD_FINAL
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("|"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), cUserDataFile);
#endif
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("|"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), gcLogDir);
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("logfile.txt"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("|"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), gcLogDir);
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("memlog.txt"));
   StringCchCatW(pdwsm->wzAdditionalFile, countof(pdwsm->wzAdditionalFile), EB("|"));

   // the Age3log.txt needs to be copied before being uploaded
   WCHAR cCopySource[MAX_PATH], cCopyDest[MAX_PATH];
   StringCchPrintfW(cCopySource, countof(cCopySource), EB("%sAge3Log.txt"), gcLogDir);
   StringCchPrintfW(cCopyDest, countof(cCopyDest), EB("%slogfile.txt"), gcLogDir);
   DeleteFileW(cCopyDest);
   CopyFileW(cCopySource, cCopyDest, FALSE);

   // same for mem log
   StringCchPrintfW(cCopySource, countof(cCopySource), EB("%sAge3mmLog.txt"), gcLogDir);
   StringCchPrintfW(cCopyDest, countof(cCopyDest), EB("%smemlog.txt"), gcLogDir);
   DeleteFileW(cCopyDest);
   CopyFileW(cCopySource, cCopyDest, FALSE);

   
   // setup exchange events and notifications
	hEventAlive = CreateEvent(&sa, FALSE, FALSE, NULL);
	hEventDone = CreateEvent(&sa, FALSE, FALSE, NULL);
	hMutex = CreateMutex(&sa, FALSE, NULL);

	if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), 
					GetCurrentProcess(), &hProc, PROCESS_ALL_ACCESS,
					TRUE, 0))
   {
#ifdef _DEBUG
      WCHAR szAssert[1024];
		bsnwprintf(szAssert, 1024, EB("Fail on DuplicateHandle (%d)"), GetLastError());
      szAssert[1023] = 0;
		MessageBoxW(sghWnd, szAssert, EB("failure"), MB_OK);
#endif
		goto LErrorCleanup;
	}

	if (hEventAlive == NULL || hEventDone == NULL || hMutex == NULL 
      || hProc == NULL)
	{
#ifdef _DEBUG
		MessageBoxW(sghWnd, EB("Fail 3"), EB("failure"), MB_OK);
#endif
		goto LErrorCleanup;
	}

	// setup interface structure. These are all based on stuff we have done above.
	pdwsm->hProc = hProc; // local reference, we use hProc for closing later
	pdwsm->pid = GetCurrentProcessId();
	pdwsm->tid = GetCurrentThreadId();
	pdwsm->hEventAlive = hEventAlive;
	pdwsm->hEventDone = hEventDone;
	pdwsm->hMutex = hMutex;
	pdwsm->dwSize = sizeof(DWSharedMem);
	pdwsm->pep = pep;
	pdwsm->eip = (DWORD_PTR) pep->ExceptionRecord->ExceptionAddress;

	// Here we choose to see a Restart checkbox. If you don't want it, just send msoctdsQuit.
	// You could send msoctdsQuit | msoctdsRecover if you want to show "Recover my work and restart ..." 
	// You don't need to set msoctdsDebug -- DW will look for a JIT and show it itself.

   pdwsm->bfmsoctdsOffer = msoctdsQuit;

//#ifdef BUILD_FINAL
//   pdwsm->bfmsoctdsOffer = msoctdsRestart;
//#else
//   pdwsm->bfmsoctdsOffer = msoctdsQuit;
//#endif

	// Testcrash only wants to hear back if the user hit Quit (or implicitly, Debug).
	// Otherwise kill the process (in our case, since we offer Restart, DW will do that for us).
//	pdwsm->bfmsoctdsLetRun = msoctdsQuit;
	pdwsm->bfmsoctdsLetRun = msoctdsQuit | msoctdsDebug;

	// don't set fDwOfficeApp if you're not one.
	// don't set fDwUsePrivacyHTA if you don't ship one
	pdwsm->bfDWBehaviorFlags = fDwReportChoice | fDwUseHKLM;

   // Get the application name from the callback if possible.
   if(sgpAppNameFunc)
      StringCchCopyW(pdwsm->wzFormalAppName, countof(pdwsm->wzFormalAppName), sgpAppNameFunc());
   else
   {
      // If no callback was registered, just grab the current module name.
	   DWORD size=GetModuleFileNameW(0, pdwsm->wzFormalAppName, DW_APPNAME_LENGTH);
      if(size==DW_APPNAME_LENGTH)
         pdwsm->wzFormalAppName[DW_APPNAME_LENGTH-1]=0;
   }

	// update me!	
   StringCchCopyA(pdwsm->szRegSubPath, countof(pdwsm->szRegSubPath), "Microsoft\\PCHealth\\ErrorReporting\\DW");

	// don't update unless you install your own or something....
   StringCchCopyA(pdwsm->szPIDRegKey, countof(pdwsm->szPIDRegKey), "HKLM\\Software\\Microsoft\\Internet Explorer\\Registration\\DigitalProductID");

	// switch to watson.microsoft.com when you're ready to ship.
	// debug and retail versions of testcrash/dw can report to officewatson.
	// only retail versions of dw can report to watson.microsoft.com
   StringCchCopyA(pdwsm->szServer, countof(pdwsm->szServer), "watson.microsoft.com");

	// Here's a 9x-safe way to get your module file name.
	// You can use GetModuleFileNameW if you know you'll only run on NT flavors
   //-- Actually UnicoWS.lib lets us do this.
	GetModuleFileNameW(NULL, pdwsm->wzModuleFileName, DW_MAX_PATH);
//	MultiByteToWideChar(CP_ACP, 0, szModPath, -1, pdwsm->wzModuleFileName, DW_MAX_PATH);

	// Do not hardcode this, but rather use your vlcidUI value.
	// Also, do not send us 1033 universally and swap dlls underneath us to simplify your setup logic.  
	// We use this to determine which privacy policy to display, so it has to be right.

   dwlen = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILANGUAGE, buffer, 256);

   if (dwlen)
   {
      WCHAR* cp = 0;

      dwCodePage = wcstol(buffer, &cp, 16);
   }

   pdwsm->lcidUI = dwCodePage;

	// Figure out where you want to look for DW.  This implementation assumes it's next to the app.
	GetModuleFileNameW(NULL, szAppFolderPath, MAX_PATH);	
	pch = szAppFolderPath + wcslen(szAppFolderPath);
			
	// chomp exe name
	while (pch >= szAppFolderPath && *pch != EB('\\'))
		pch--;

	if (pch < szAppFolderPath)
   {
#ifdef _DEBUG
		MessageBoxW(sghWnd, EB("Fail 4: generating full path to DW.exe"), EB("failure"), MB_OK);
#endif
		goto LErrorCleanup;
	}

	*pch = '\0'; // prune

   #ifndef BUILD_FINAL
   // HI EVERYBODY!!  It's time to call DrNick.  Then we'll let dr watson go.
   StringCchPrintfW(szApplicationName, countof(szApplicationName), EB("%s\\drnick.exe"), szAppFolderPath);
   StringCchPrintfW(szCommandLine, countof(szCommandLine), EB(" %u"), (DWORD) hFileMap);
	processCreated = CreateProcessW(szApplicationName, szCommandLine, NULL, NULL, TRUE, 
				  CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL,
				  NULL, &si, &pi);
   // Wait for it to end.
   if(processCreated)
   {
      WaitForSingleObject(pi.hProcess, INFINITE);

      // Close handles.
      CloseHandle(pi.hProcess);
	   CloseHandle(pi.hThread);
   }
   #endif


	// it's better security to set a full path for the app than to let the OS go looking for it.
   StringCchPrintfW(szApplicationName, countof(szApplicationName), EB("%s\\dw15.exe"), szAppFolderPath);
   StringCchPrintfW(szCommandLine, countof(szCommandLine), EB(" -x -s %u"), (DWORD) hFileMap);


	memset(&si, 0, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW);
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));

   // notify the caller (if available) to get rid of the "hold your panties" dialog

   if (sgpExceptionWaitDlg)
      sgpExceptionWaitDlg(false, 0, 0);

   // call watson!

	if (CreateProcessW(szApplicationName, szCommandLine, NULL, NULL, TRUE, 
				  CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL,
				  NULL, &si, &pi))
	{
		fDwRunning = TRUE;
		while (fDwRunning)
		{
			if (WaitForSingleObject(hEventAlive, DW_TIMEOUT_VALUE) == WAIT_OBJECT_0)
			{
				if (WaitForSingleObject(hEventDone, 1) == WAIT_OBJECT_0)
				{
					fDwRunning = FALSE;
				}
			   continue;
			}
				
			 // we timed-out waiting for DW to respond, try to quit
			dw = WaitForSingleObject(hMutex, DW_TIMEOUT_VALUE);
			if (dw == WAIT_TIMEOUT)
				fDwRunning = FALSE; // either DW's hung or crashed, we must carry on  
			else if (dw == WAIT_ABANDONED)
			{
				fDwRunning = FALSE;
				ReleaseMutex(hMutex);
			}
			else
			{
				// DW has not woken up?
				if (WaitForSingleObject(hEventAlive, 1) != WAIT_OBJECT_0)
					// tell DW we're through waiting for it's sorry self
				{
					SetEvent(hEventDone);
					fDwRunning = FALSE;
				}
				else
				{
					// are we done
					if (WaitForSingleObject(hEventDone, 1) == WAIT_OBJECT_0)
						fDwRunning = FALSE;
				}
				ReleaseMutex(hMutex);
			}
		}

		// no, clean up
		CloseHandle(hEventAlive);
		hEventAlive = NULL;
		CloseHandle(hEventDone);
		hEventDone = NULL;
		CloseHandle(hMutex);
		hMutex = NULL;
		CloseHandle(hProc);
		hProc = NULL;
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	} // end if CreateProcess succeeded
	
	msoctdsResult = pdwsm->msoctdsResult;

	UnmapViewOfFile(pdwsm);
	pdwsm = NULL;
	CloseHandle(hFileMap);
	hFileMap = NULL;

	// we use a local to let the file map etc. get cleaned up first.
	if (msoctdsResult == msoctdsDebug)
		return EXCEPTION_CONTINUE_SEARCH;

	//return EXCEPTION_EXECUTE_HANDLER;
   // jce [11/19/2002] -- changed this from above so the XP watson thing doesn't also come up.
   doFatalExit(0);

LErrorCleanup:

   // notify the caller (if available) to put up a "hold your panties" dialog

   if (sgpExceptionWaitDlg)
      sgpExceptionWaitDlg(false, 0, 0);

   if (hEventAlive)
		CloseHandle(hEventAlive);
	if (hEventDone)
		CloseHandle(hEventDone);
	if (hMutex)
		CloseHandle(hMutex);
	if (hProc)
		CloseHandle(hProc);
	if (pdwsm)
		UnmapViewOfFile(pdwsm);
	if (hFileMap)
		CloseHandle(hFileMap);
#endif
	return EXCEPTION_EXECUTE_HANDLER;
}

//==============================================================================
// GetExceptionString
//==============================================================================
const WCHAR *GetExceptionString( DWORD dwCode )
{
    #define EXCEPTION( x ) case EXCEPTION_##x: return (L#x);

    switch ( dwCode )
    {
        EXCEPTION( ACCESS_VIOLATION )
        EXCEPTION( DATATYPE_MISALIGNMENT )
        EXCEPTION( BREAKPOINT )
        EXCEPTION( SINGLE_STEP )
        EXCEPTION( ARRAY_BOUNDS_EXCEEDED )
        EXCEPTION( FLT_DENORMAL_OPERAND )
        EXCEPTION( FLT_DIVIDE_BY_ZERO )
        EXCEPTION( FLT_INEXACT_RESULT )
        EXCEPTION( FLT_INVALID_OPERATION )
        EXCEPTION( FLT_OVERFLOW )
        EXCEPTION( FLT_STACK_CHECK )
        EXCEPTION( FLT_UNDERFLOW )
        EXCEPTION( INT_DIVIDE_BY_ZERO )
        EXCEPTION( INT_OVERFLOW )
        EXCEPTION( PRIV_INSTRUCTION )
        EXCEPTION( IN_PAGE_ERROR )
        EXCEPTION( ILLEGAL_INSTRUCTION )
        EXCEPTION( NONCONTINUABLE_EXCEPTION )
        EXCEPTION( STACK_OVERFLOW )
        EXCEPTION( INVALID_DISPOSITION )
        EXCEPTION( GUARD_PAGE )
        EXCEPTION( INVALID_HANDLE )
    }

    // If not one of the "known" exceptions, try to get the string
    // from NTDLL.DLL's message table.

    static WCHAR szBuffer[512] = { 0 };

//FIXME
#ifndef XBOX
    FormatMessageW(  FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                    GetModuleHandleW(EB("NTDLL.DLL")),
                    dwCode, 0, szBuffer, 512, 0 );
#endif

    return szBuffer;

} // GetExceptionString


//==============================================================================
// setExceptionDirectories
//==============================================================================
void setExceptionDirectories(const WCHAR *logDir, const WCHAR *exeDir)
{
   if (!logDir || !exeDir)
   {
      BASSERT(0);
      return;
   }

   bsnwprintf(gcLogDir, MAX_PATH, EB("%s"), logDir);
   bsnwprintf(gcExeDir, MAX_PATH, EB("%s"), exeDir);
} // setExceptionDirectories

//==============================================================================
// exceptionHandler
//==============================================================================
LONG WINAPI exceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
//long _stdcall ProcessException( EXCEPTION_POINTERS* pExceptionInfo)
{
   static bool bInDebugger = false;


//MessageBoxW(sghWnd, EB("in handler"), EB("debug"), MB_OK);
//
//__asm int 3;


   // Call the pre-watson callback (which might minimize the game, flush log files, etc.)
   if(sgpPreFunc)
      sgpPreFunc();

   // handle situations depending on we are in or out of debugger, or what kind of problem it is
   if (IsDebuggerPresent() || bInDebugger)
		return EXCEPTION_CONTINUE_SEARCH;

   bInDebugger = true;

   if ((EXCEPTION_BREAKPOINT==pExceptionInfo->ExceptionRecord->ExceptionCode) || 
       (EXCEPTION_SINGLE_STEP==pExceptionInfo->ExceptionRecord->ExceptionCode))
		return EXCEPTION_CONTINUE_EXECUTION;

   // call watson.
   watsonExceptionHandler(pExceptionInfo);

   bInDebugger = false;
   
   if (gpPreviousExceptionFilter)
     return gpPreviousExceptionFilter(pExceptionInfo);

   return EXCEPTION_CONTINUE_SEARCH;

} // exceptionHandler

//==============================================================================
// initExceptionHandler
//==============================================================================
void initExceptionHandler(void)
{
   // set the handler
   if (!IsDebuggerPresent())
      gpPreviousExceptionFilter = SetUnhandledExceptionFilter(exceptionHandler);

} // initExceptionHandler

//==============================================================================
// shutdownExceptionHandler
//==============================================================================
void shutdownExceptionHandler(void)
{
   if (gpPreviousExceptionFilter)
   {
      SetUnhandledExceptionFilter(gpPreviousExceptionFilter);

      gpPreviousExceptionFilter = 0;
   }
   
} // shutdownExceptionHandler

//==============================================================================
// eof: exception.cpp
//==============================================================================
