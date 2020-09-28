//-----------------------------------------------------------------------------
// system_win32.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef SYSTEM_WIN32
#define SYSTEM_WIN32

#include "common/core/system.h"

namespace gr 
{
	class Win32System : public System
	{
	public:
		Win32System();
		virtual ~Win32System();
    
		virtual bool debuggerPresent(void);
	};

	extern Win32System& gWin32System;

} // namespace gr

#endif // SYSTEM_WIN32