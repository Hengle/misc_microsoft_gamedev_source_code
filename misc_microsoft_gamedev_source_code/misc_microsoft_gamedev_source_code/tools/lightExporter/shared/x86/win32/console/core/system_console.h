//-----------------------------------------------------------------------------
// File: system_console.h
// Console specific System class.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef SYSTEM_CONSOLE
#define SYSTEM_CONSOLE

#include "x86/win32/core/system_win32.h"

namespace gr 
{
	class ConsoleSystem : public Win32System
	{
	public:
		ConsoleSystem();
		virtual ~ConsoleSystem();
    			
		virtual void abort(void);
    
    virtual void message(MessageLevel level, const char* pMessage);

		// true to ignore
		virtual bool assertEvent(const char* Pfile, int line, const char* Pexpr);

		// true to ignore
		virtual bool verifyEvent(const char* Pfile, int line, const char* Pexpr);
	};

	extern ConsoleSystem& gConsoleSystem;

} // namespace gr

#endif // SYSTEM_CONSOLE