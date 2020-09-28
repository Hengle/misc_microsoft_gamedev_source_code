//============================================================================
// xexception.cpp
// Copyright (c) 2002, Blue Shift, Inc.
// Copyright (c) 2005, Ensemble Studios
//============================================================================
#include "xcore.h"
#include "file\xboxFileUtils.h"
#include "xexception.h"
#include "xdebugtext.h"
#include "xdb\xdbManager.h"
#include "threading\eventDispatcher.h"
#include <xbdm.h>

// [11/14/2008 xemu] sucks, but needed to flush the fps log
//#include "..\..\xgame\modemanager.h"

#ifdef USE_BUILD_INFO
#include "..\..\xgame\build_info.inc"
#endif

#pragma warning(disable:4702)

#ifdef BUILD_FINAL
   #define CHECK_FOR_DEBUGGER 0
#else
   #define CHECK_FOR_DEBUGGER 1
#endif

XEXCEPTION_CALLBACK_FUNC X_Exception::mCallBackFunc = NULL;
volatile long X_Exception::mHandlingMeltdown;

bool X_Exception::mUseCacheDrive = false;
bool X_Exception::mFirstWrite = true;

namespace {

	struct
	{
		DWORD code;
		char *Pmsg;
	} G_exception_strings[] =
	{

		{ EXCEPTION_ACCESS_VIOLATION,					"EXCEPTION_ACCESS_VIOLATION:\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access." },
		{ EXCEPTION_ARRAY_BOUNDS_EXCEEDED,		   "EXCEPTION_ARRAY_BOUNDS_EXCEEDED:\nThe thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking." },
		{ EXCEPTION_BREAKPOINT,							"EXCEPTION_BREAKPOINT:\nA breakpoint was encountered." },
		{ EXCEPTION_DATATYPE_MISALIGNMENT,		   "EXCEPTION_DATATYPE_MISALIGNMENT:\nThe thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on." },
		{ EXCEPTION_FLT_DENORMAL_OPERAND,			"EXCEPTION_FLT_DENORMAL_OPERAND:\nOne of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value." },
		{ EXCEPTION_FLT_DIVIDE_BY_ZERO,				"EXCEPTION_FLT_DIVIDE_BY_ZERO:\nThe thread tried to divide a floating-point value by a floating-point divisor of zero." },
		{ EXCEPTION_FLT_INEXACT_RESULT,				"EXCEPTION_FLT_INEXACT_RESULT:\nThe result of a floating-point operation cannot be represented exactly as a decimal fraction." },
		{ EXCEPTION_FLT_INVALID_OPERATION,		   "EXCEPTION_FLT_INVALID_OPERATION:\nThis exception represents any floating-point exception not included in this list." },
		{ EXCEPTION_FLT_OVERFLOW,						"EXCEPTION_FLT_OVERFLOW:\nThe exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type." },
		{ EXCEPTION_FLT_STACK_CHECK,					"EXCEPTION_FLT_STACK_CHECK:\nThe stack overflowed or underflowed as the result of a floating-point operation." },
		{ EXCEPTION_FLT_UNDERFLOW,						"EXCEPTION_FLT_UNDERFLOW:\nThe exponent of a floating-point operation is less than the magnitude allowed by the corresponding type." },
		{ EXCEPTION_ILLEGAL_INSTRUCTION,			   "EXCEPTION_ILLEGAL_INSTRUCTION:\nThe thread tried to execute an invalid instruction." },
		{ EXCEPTION_IN_PAGE_ERROR,						"EXCEPTION_IN_PAGE_ERROR:\nThe thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network." },
		{ EXCEPTION_INT_DIVIDE_BY_ZERO,				"EXCEPTION_INT_DIVIDE_BY_ZERO:\nThe thread tried to divide an integer value by an integer divisor of zero." },
		{ EXCEPTION_INT_OVERFLOW,						"EXCEPTION_INT_OVERFLOW:\nThe result of an integer operation caused a carry out of the most significant bit of the result." },
		{ EXCEPTION_INVALID_DISPOSITION,			   "EXCEPTION_INVALID_DISPOSITION:\nAn exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception." },
		{ EXCEPTION_NONCONTINUABLE_EXCEPTION,     "EXCEPTION_NONCONTINUABLE_EXCEPTION:\nThe thread tried to continue execution after a noncontinuable exception occurred." },
		{ EXCEPTION_PRIV_INSTRUCTION,					"EXCEPTION_PRIV_INSTRUCTION:\nThe thread tried to execute an instruction whose operation is not allowed in the current machine mode." },
		{ EXCEPTION_SINGLE_STEP,						"EXCEPTION_SINGLE_STEP:\nA trace trap or other single-instruction mechanism signaled that one instruction has been executed." },
		{ EXCEPTION_STACK_OVERFLOW,					"EXCEPTION_STACK_OVERFLOW:\nThe thread used up its stack." },
	};

	const int NUM_EXCEPTIONS = (sizeof(G_exception_strings) / sizeof(G_exception_strings[0]));

	static char* GetExceptionName(DWORD code)
	{
		for (int i = 0; i < NUM_EXCEPTIONS; i++)
			if (G_exception_strings[i].code == code)
				return G_exception_strings[i].Pmsg;

		static char msg[64];
		sprintf_s(msg, sizeof(msg), "Unknown exception: %X", code);
		return msg;
	}

}  // end anonymous namespace

extern "C"
{
   void MapFileLookupFunc(void)
   {
   }
}

//==============================================================================
// X_Exception::DumpFailReport
//==============================================================================
void X_Exception::DumpFailReport(const char* cpErrorMsg, const char* cpFile)
{
   // using the XContent APIs, store the error to the given file name
   if (!TestCacheDrive())
      return;

   if (!cpErrorMsg || strlen(cpErrorMsg) == 0)
      return;

   if (!cpFile)
      return;

   char buffer[256];
   StringCchPrintfA(buffer, 255, "cache:\\logs\\%s", cpFile);

   DWORD dwCreationDisposition = OPEN_ALWAYS;
   if (mFirstWrite)
   {
      dwCreationDisposition = CREATE_ALWAYS;
      mFirstWrite = false;
   }

   HANDLE hFile = CreateFileA(buffer, GENERIC_WRITE, 0, 0, dwCreationDisposition, 0, 0);
   if (hFile != INVALID_HANDLE_VALUE)
   {
      SetFilePointer(hFile, 0, 0, FILE_END);
      DWORD bytesWritten;
      WriteFile(hFile, cpErrorMsg, strlen(cpErrorMsg), &bytesWritten, 0);
      bytesWritten;
      CloseHandle(hFile);

#ifdef XBOX
      XFlushUtilityDrive();
#endif
   }
}

//==============================================================================
// X_Exception::Meltdown
//==============================================================================
void X_Exception::Meltdown(const char* Pmsg, bool die)
{
   InterlockedExchange(&mHandlingMeltdown, TRUE);

   DumpFailReport(Pmsg, "PhoenixMeltdown.txt");
   //BModeManager::shutdownFPSLogFile(false);

#ifndef BUILD_FINAL
//   DmMapDevkitDrive();
   if ((Pmsg) && (strlen(Pmsg)))
   {
      HANDLE handle = CreateFileA("d:\\PhoenixMeltdown.txt", GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
      if (INVALID_HANDLE_VALUE != handle)
      {
         //char buffer[256];
         SetFilePointer(handle, 0, 0, FILE_END);
         DWORD bytesWritten;
         WriteFile(handle, Pmsg, strlen(Pmsg), &bytesWritten, 0);
         CloseHandle(handle);
      }
   }      
#endif

   // Exception callback
   if( mCallBackFunc )
   {
      mCallBackFunc( Pmsg, die );
   }
   else
   {
      trace( "Failed to write fail report to dump directory look on 360 drive.\n" );
   }

#if CHECK_FOR_DEBUGGER
#ifndef BUILD_FINAL
	OutputDebugStringA("\n***** X_Exception::Meltdown: ");
	if ((Pmsg) && (strlen(Pmsg)))
	   OutputDebugStringA(Pmsg);
		
	if (DmIsDebuggerPresent())
		DebugBreak();
#endif
#endif

	static volatile LONG insideMeltdownFunc;
	if (insideMeltdownFunc)
	{
		for ( ; ; )
			Sleep(1);
	}
	
	InterlockedExchange(&insideMeltdownFunc, TRUE);
	
#ifndef BUILD_FINAL
   int y = 0;
   // Wait a while for the GPU to finish processing and resolving to the front buffer.
   Sleep(150);      
   
	if ((Pmsg) && (strlen(Pmsg)))
	   y = G_debug_text.render_cooked(0, 0, Pmsg);
#endif
      
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

#ifndef BUILD_FINAL
   G_debug_text.render_cooked(0, y+1, "Execution Cannot Continue - Press Start to Reboot");

   Sleep(500);

   // Now wait for a button 
   for ( ; ; )
   {
      XINPUT_STATE state;
      DWORD res = XInputGetState(0, &state);
      if (ERROR_SUCCESS == res)
      {
         const bool buttonStart = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;

         if (buttonStart)
         {
            DmReboot(DMBOOT_TITLE);
         }
      }
      Sleep(16);
   }
#endif   

	if (die)
	{
		for ( ; ; )
		{
			Sleep(1);
		}		
	}

	InterlockedExchange(&insideMeltdownFunc, FALSE);
	
	InterlockedExchange(&mHandlingMeltdown, FALSE);
}

//==============================================================================
// X_Exception::ExceptionFilter
//==============================================================================
long X_Exception::ExceptionFilter(LPEXCEPTION_POINTERS pe)
{
   static bool inside_evaluate;
   bool debugger_present = false;

#if CHECK_FOR_DEBUGGER
#ifndef BUILD_FINAL
   debugger_present = (DmIsDebuggerPresent() != FALSE);
#endif
#endif

   if ((inside_evaluate) || (debugger_present))
      return EXCEPTION_CONTINUE_SEARCH;

   inside_evaluate = true;

   // Static to prevent stack overflows
   static char buf[4096];
   SYSTEMTIME t;

   GetLocalTime(&t);

   int numChars = sprintf_s(buf, sizeof(buf), "== MELT == %04d%02d%02dT%02d%02d%02d ==\n", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
#ifdef USE_BUILD_INFO
   numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "DEPOT:"DEPOT_REVISION"|BUILD:"BUILD_NUMBER"|CHANGELIST:"CHANGELIST_NUMBER"\n");
#else
   numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "NO_BUILD_INFO\n");
#endif

#if defined(XBOX)
   #if !defined(BUILD_FINAL)
      DM_XBE xbeInfo;
      HRESULT hr = DmGetXbeInfo(NULL, &xbeInfo);
      if (hr == XBDM_NOERR)
      {
         numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "XBE Launch Path: %s\n", xbeInfo.LaunchPath);
         numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "XBE TimeStamp: %d\n", xbeInfo.TimeStamp);
         numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "XBE CheckSum: %d\n", xbeInfo.CheckSum);
      }
      static char dmXboxName[256];
      DWORD size = sizeof(dmXboxName);
      hr = DmGetXboxName(dmXboxName, &size);
      if (hr == XBDM_NOERR)
      {
         numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "Xbox Name: %s\n", dmXboxName);
      }
   #endif

   CHAR gamerTag[XUSER_NAME_SIZE];
   for (DWORD i=0; i < 4; ++i)
   {
      if (XUserGetName(i, gamerTag, XUSER_NAME_SIZE) == ERROR_SUCCESS)
      {
         numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "User (%d): %s\n", i, gamerTag);
      }
   }
#endif

   MEMORYSTATUS memStatus;
   GlobalMemoryStatus(&memStatus);
   numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "Current Free: %u\n", memStatus.dwAvailPhys);

   numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "== == ==\nUnhandled Exception\n\n%s\nInstruction Pointer: 0x%08X",
               GetExceptionName(pe->ExceptionRecord->ExceptionCode),
               pe->ExceptionRecord->ExceptionAddress);

#if 0
   sprintf_s(buf + strlen(buf), sizeof(buf), "\nTracer: %i", G_trace_id);
#endif

   if (pe->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
   {
      const DWORD type = pe->ExceptionRecord->ExceptionInformation[0];
      const DWORD addr = pe->ExceptionRecord->ExceptionInformation[1];

      numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "\n\nAccess Type: %s\nAddress: 0x%08X", (type == 0) ? "READ" : "WRITE", addr);
   }

   numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "\nMapFileLookupFunc() address, for .map file lookup: 0x%08X\n", MapFileLookupFunc);
   numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "Thread Handle: 0x%X, ID: 0x%X, Index: %i\n", GetCurrentThread(), GetCurrentThreadId(), gEventDispatcher.getThreadIndex());

#if 0   
   MEMORYSTATUS memStatus;
   GlobalMemoryStatus(&memStatus);
   numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "Current Free: %u", memStatus.dwAvailPhys);
#endif   
      
   gXDBManager.beginStackTrace(pe);

   const int stackTraceLevels = gXDBManager.getStackTraceLevels();   
   if (stackTraceLevels > 0)
   {
      numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "Stack trace levels: %u\n", stackTraceLevels);

      const BXDBFileReader::BLookupInfo& info = gXDBManager.getXDBLookupInfo();
      for (int i = 0; i < stackTraceLevels && numChars < 3900; i++)
      {
         if (!gXDBManager.walkStackTrace(i))
            numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "0x%08X\n", gXDBManager.getStackTraceAddress(i));
         else
            numChars += sprintf_s(buf + numChars, sizeof(buf) - numChars, "%s(%i): %s (0x%08X)\n", info.mFilename.c_str(), info.mLine, info.mSymbol.c_str(), gXDBManager.getStackTraceAddress(i));   
      }
   }

   Meltdown(buf, false);

   inside_evaluate = false;

   return EXCEPTION_CONTINUE_SEARCH;
}

//==============================================================================
// X_Exception::Init
//==============================================================================
void X_Exception::Init()
{
   SetCallback(ExceptionCallback);
   SetUnhandledExceptionFilter(ExceptionFilter);

   // attempt to mount the utility drive
   //
#ifdef XBOX
   BXboxFileUtils::initXboxCachePartition();

   TestCacheDrive();
#endif
}

//==============================================================================
// X_Exception::TestCacheDrive
//==============================================================================
bool X_Exception::TestCacheDrive()
{
#ifdef XBOX
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
// X_Exception::ExceptionCallback
//==============================================================================
void X_Exception::ExceptionCallback(const char* cpMsg, BOOL die)
{
   __try
   {
      DumpFailReport(cpMsg, "PhoenixMeltdown.txt");
      //BModeManager::shutdownFPSLogFile(false);
   }
   __except( EXCEPTION_EXECUTE_HANDLER )
   {
   }
}
