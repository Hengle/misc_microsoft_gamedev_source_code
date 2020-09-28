//==============================================================================
// trace.h
//
// Copyright (c) 1999-2006, Ensemble Studios
//==============================================================================

// rg [7/21/05] Removing pragma to get rid of a bug in incredibuild 2.40 beta  that causes tons of warnings
//#pragma once

//==============================================================================
#ifndef __TRACE_H__
#define __TRACE_H__

void trace(const char *lpszFormat, ...);
void tracenocrlf(const char *lpszFormat, ...);
void traceV(const char *lpszFormat, va_list args, bool crlf = true);

typedef void (*BTraceCallback)(const char* pMsg);

void setTraceCallback(BTraceCallback pCallback);

#endif // __TRACE_H__