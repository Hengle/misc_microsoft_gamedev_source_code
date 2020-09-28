//-----------------------------------------------------------------------------
// File: system_dx9.h
// Console specific System class.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef SYSTEM_DX9
#define SYSTEM_DX9

#include "x86/win32/core/win_hdr.h"
#include "x86/win32/core/system_win32.h"

namespace gr 
{
	class DX9System : public Win32System
	{
		HWND hMainWindow;
		HANDLE mStdout;
		bool mConsoleWindow;
		
	public:
		DX9System();
		virtual ~DX9System();
    			
		virtual void abort(void);
    
    virtual void message(MessageLevel level, const char* pMessage);

		// true to ignore
		virtual bool assertEvent(const char* Pfile, int line, const char* Pexpr);

		// true to ignore
		virtual bool verifyEvent(const char* Pfile, int line, const char* Pexpr);

		void setMainWindow(HWND hMainWindow);
		HWND getMainWindow(void) const;
		
		void enableConsoleWindow(void);
	};

	extern DX9System& gDX9System;

} // namespace gr

#endif // SYSTEM_DX9