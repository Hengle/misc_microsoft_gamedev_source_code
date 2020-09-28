//-----------------------------------------------------------------------------
// File: logfile.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef LOGFILE_H

#include "common/core/core.h"
#include "common/utils/stream.h"

namespace gr
{

	class LogFile
	{
	public:
		LogFile(bool traceEcho = false) : 
			mTraceEcho(traceEcho), 
			mCurIndent(0)
		{
		}

		LogFile(const char* pFilename) : 
			mTraceEcho(false),
			mCurIndent(0)
		{
			mStream.open(pFilename, false, false);
		}

		~LogFile()
		{
			close();
		}

		void close(void)
		{
			mStream.close();
		}	

		bool isValid(void)
		{
			return mStream.isOpened();
		}

		bool traceEcho(void) const
		{
			return mTraceEcho;
		}

		void setTraceEcho(bool traceEcho)
		{
			mTraceEcho = traceEcho;
		}

		// true on failure
		bool open(const char* pFilename)
		{
			close();
			return mStream.open(pFilename, false, false);
		}

		LogFile& printf(const char* pMsg, ...)
		{
			if ((!mStream.isOpened()) && (!mTraceEcho))
				return *this;

			va_list args;
			va_start(args, pMsg);
			char buf[1024];
			memset(buf, ' ', DebugRange(mCurIndent, sizeof(buf)));
			_vsnprintf(buf + mCurIndent, sizeof(buf), pMsg, args);
			va_end(args);

			if (mStream.isOpened())
				mStream.writeBytes(buf, static_cast<int>(strlen(buf)));

			if (mTraceEcho)
				gSystem.message(System::STATUS, buf);
			
			return *this;
		}

		FILEStream& getStream(void) 
		{
			return mStream;
		}

		operator FILEStream&() 
		{
			return mStream;
		}
		
		void indent(int i)
		{
			mCurIndent += i * 2;
		}

	private:
		FILEStream mStream;
		bool mTraceEcho;
		int mCurIndent;
	};
} // namespace gr

#endif // LOGFILE_H
