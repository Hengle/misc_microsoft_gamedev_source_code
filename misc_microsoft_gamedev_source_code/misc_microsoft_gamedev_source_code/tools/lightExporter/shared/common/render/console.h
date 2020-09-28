//-----------------------------------------------------------------------------
// File: console.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef CONSOLE_H
#define CONSOLE_H

#include "common/utils/string.h"
#include <string>
#include <list>

namespace gr
{
	class Console
	{
	protected:
		typedef std::string StringType;
		typedef std::list<StringType> LineListType;

		LineListType mLines;
    	
	public:
		Console()
		{
		}

		virtual ~Console()
		{
		}

		void clear(void)
		{
			mLines.clear();
		}

		void puts(const char* pStr)
		{
			mLines.push_back(pStr);
		}

		void printf(const char* pFmt, ...)
		{
			char buf[1024];

			va_list args;
			va_start(args, pFmt);
			_vsnprintf(buf, sizeof(buf), pFmt, args);
			va_end(args);
			
			puts(buf);
		}

		virtual void display(void)
		{
		}
	};

	extern Console& gConsole;

} // namespace gr

#endif // CONSOLE_H


