//	ILogFileWriter : Interface to object that writes vince log data to a file
//
//	Created 2007/01/22 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.

#pragma once

#ifdef _XBOX
	#include <xtl.h>
#else
	#include <windows.h>
#endif

namespace Vince
{
	class ILogFileWriter
	{
	public:
        virtual ~ILogFileWriter() {};

        // Write vince data to the log file
        virtual HRESULT WriteToLogFile(const char* cstrLogFileName, const char* cData, int count) = 0;	
	};
}