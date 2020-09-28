//-----------------------------------------------------------------------------
// File: system_dx9.cpp
// DX9 specific System class.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "system_dx9.h"

namespace gr 
{
	DX9System::DX9System() : 
		Win32System(), 
		hMainWindow(0),
		mConsoleWindow(0),
		mStdout(INVALID_HANDLE_VALUE)
	{
	}
	
	DX9System::~DX9System()
	{
	}
    			
	void DX9System::abort(void)
	{
		exit(EXIT_FAILURE);
	}
    
  void DX9System::message(MessageLevel level, const char* pMessage)
  {
		if (mConsoleWindow)
			fputs(pMessage, stdout);
  
		if ((level >= FATAL) && (level <= WARNING))
		{
			const char* pTitles[] = 
			{
				"Error",
				"Verify() Failed",
				"Assert() Failed",
				"Warning"
			};
			const int NumMessages = sizeof(pTitles) / sizeof(pTitles[0]);

			int tIndex = level - FATAL;
			if (tIndex >= NumMessages)
				tIndex = 0;

			MessageBox(hMainWindow, pMessage, pTitles[tIndex], MB_OK);
		}
		else
		{
			OutputDebugString(pMessage);
		}
  };

	// true to ignore
	bool DX9System::assertEvent(const char* Pfile, int line, const char* Pexpr)
	{
		if (debuggerPresent())
			return false;

		char buf[1024];
		_snprintf(buf, sizeof(buf), 
			"%s(%i): Assert() failed! Expression: %s",
			Pfile, line, Pexpr);

		message(System::ASSERT, buf);
		
		abort();

		return false;
	}

	// true to ignore
	bool DX9System::verifyEvent(const char* Pfile, int line, const char* Pexpr)
	{
		if (debuggerPresent())
			return false;

		char buf[1024];
		
		_snprintf(buf, sizeof(buf), 
			"%s(%i): Verify() failed! Expression: %s",
			Pfile, line, Pexpr);

		message(System::VERIFY, buf);
		
		abort();

		return false;
	}

	void DX9System::setMainWindow(HWND hWindow)
	{
		hMainWindow = hWindow;
	}
	
	HWND DX9System::getMainWindow(void) const
	{
		return hMainWindow;
	}
				
	void DX9System::enableConsoleWindow(void)
	{
		BOOL status = AllocConsole();
		mConsoleWindow = (status != 0);
		mStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		freopen("CONOUT$", "w+", stdout);
		freopen("CONOUT$", "w+", stderr);
	}
  
  DX9System gDX9SystemObject;

	System& gSystem = gDX9SystemObject;
	Win32System& gWin32System = gDX9SystemObject;
	DX9System& gDX9System = gDX9SystemObject;

} // namespace gr

