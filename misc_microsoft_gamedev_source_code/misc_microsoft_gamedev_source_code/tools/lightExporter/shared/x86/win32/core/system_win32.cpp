//-----------------------------------------------------------------------------
// File: system_win32.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "system_win32.h"

#include "x86/win32/core/win_hdr.h"

namespace gr 
{
	Win32System::Win32System()
	{
	}

	Win32System::~Win32System()
	{
	}
	
	//FIXME
	bool Win32System::debuggerPresent(void)
	{
		return IsDebuggerPresent() != 0;
	}
		
} // namespace gr


