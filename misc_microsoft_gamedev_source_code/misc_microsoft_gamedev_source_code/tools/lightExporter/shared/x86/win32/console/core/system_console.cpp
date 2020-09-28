//-----------------------------------------------------------------------------
// File: system_console.cpp
// Console specific System class.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "system_console.h"

namespace gr 
{
	ConsoleSystem::ConsoleSystem() : Win32System()
	{
	}
	
	ConsoleSystem::~ConsoleSystem()
	{
	}
    			
	void ConsoleSystem::abort(void)
	{
		exit(EXIT_FAILURE);
	}
    
  void ConsoleSystem::message(MessageLevel level, const char* pMessage)
  {
		printf("%s", pMessage);
  };

	// true to ignore
	bool ConsoleSystem::assertEvent(const char* Pfile, int line, const char* Pexpr)
	{
		if (debuggerPresent())
			return false;

		char buf[1024];
		_snprintf(buf, sizeof(buf), 
			"%s(%i): Assert() failed! Expression: %s\n",
			Pfile, line, Pexpr);

		message(System::ASSERT, buf);
		
		abort();

		return false;
	}

	// true to ignore
	bool ConsoleSystem::verifyEvent(const char* Pfile, int line, const char* Pexpr)
	{
		if (debuggerPresent())
			return false;

		char buf[1024];
		
		_snprintf(buf, sizeof(buf), 
			"%s(%i): Verify() failed! Expression: %s\n",
			Pfile, line, Pexpr);

		message(System::VERIFY, buf);
		
		abort();

		return false;
	}
  
  ConsoleSystem gConsoleSystemObject;

	System& gSystem = gConsoleSystemObject;
	Win32System& gWin32System = gConsoleSystemObject;
	ConsoleSystem& gConsoleSystem = gConsoleSystemObject;

} // namespace gr

