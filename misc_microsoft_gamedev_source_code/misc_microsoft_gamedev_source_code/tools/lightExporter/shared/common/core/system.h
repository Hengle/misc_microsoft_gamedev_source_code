//-----------------------------------------------------------------------------
// File: system.h
// Platform independent System class, which provides basic message output and OS related services.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdarg.h>

#include "common/core/core.h"

namespace gr 
{
  class System
  {
  public:
		virtual ~System() { };
    
    virtual void abort(void) = 0;
    
    enum MessageLevel
    {
      FATAL,
      VERIFY,
      ASSERT,
      ERR,
      WARNING,      
      STATUS,
      TRACE,
      NUM_MESSAGE_LEVELS
    };
    
    virtual void message(MessageLevel level, const char* pMessage) = 0;

    virtual bool debuggerPresent(void) = 0;

		// true to ignore
		virtual bool assertEvent(const char* Pfile, int line, const char* Pexpr) = 0;

		// true to ignore
		virtual bool verifyEvent(const char* Pfile, int line, const char* Pexpr) = 0;
  };
  
  extern System& gSystem;
  
  const int MESSAGE_BUFFER_SIZE = 4096;
  
  inline void Status(const char* pMessage, ...)
	{
		char buf[MESSAGE_BUFFER_SIZE];
		va_list args;
		va_start(args, pMessage);
		_vsnprintf(buf, sizeof(buf), pMessage, args);
		gSystem.message(System::STATUS, buf);
		va_end(args);
	}

#if DEBUG
	inline void Trace(const char* pMessage, ...)
	{
		char buf[MESSAGE_BUFFER_SIZE];
		va_list args;
		va_start(args, pMessage);
		_vsnprintf(buf, sizeof(buf), pMessage, args);
		gSystem.message(System::TRACE, buf);
		va_end(args);
	}
#else
	inline void Trace(const char* pMessage, ...)
	{
		pMessage;
	}
#endif

	inline void TraceAlways(const char* pMessage, ...)
	{
		char buf[MESSAGE_BUFFER_SIZE];
		va_list args;
		va_start(args, pMessage);
		_vsnprintf(buf, sizeof(buf), pMessage, args);
		gSystem.message(System::TRACE, buf);
		va_end(args);
	}

	inline void Error(const char* pMessage, ...)
	{
		char buf[MESSAGE_BUFFER_SIZE];
		va_list args;
		va_start(args, pMessage);
		_vsnprintf(buf, sizeof(buf), pMessage, args);
		gSystem.message(System::FATAL, buf);
		va_end(args);
	}

	inline void Warning(const char* pMessage, ...)
	{
		char buf[MESSAGE_BUFFER_SIZE];
		va_list args;
		va_start(args, pMessage);
		_vsnprintf(buf, sizeof(buf), pMessage, args);
		gSystem.message(System::WARNING, buf);
		va_end(args);
	}

} // namespace gr

#endif // SYSTEM_H

