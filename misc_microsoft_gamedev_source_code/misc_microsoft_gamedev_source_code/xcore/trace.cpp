//============================================================================
//
// File: trace.cpp
//  
// Copyright (c) 1999-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "consoleOutput.h"

const int cLargeBufferSize = 8192;

BTraceCallback gTraceCallback;

//==============================================================================
// trace
//==============================================================================
void trace(const char *lpszFormat, ...)
{
   va_list args;
   va_start(args, lpszFormat);

   traceV(lpszFormat, args);

   va_end(args);

} // trace

//==============================================================================
// tracenocrlf
//==============================================================================
void tracenocrlf(const char *lpszFormat, ...)
{
   va_list args;
   va_start(args, lpszFormat);

   traceV(lpszFormat, args, false);

   va_end(args);

} // tracenocrlf

//==============================================================================
// trace
//==============================================================================
void traceV(const char *lpszFormat, va_list args, bool crlf)
{
   char szBuffer[cLargeBufferSize];
   HRESULT hr = StringCchVPrintfA(szBuffer, cLargeBufferSize, lpszFormat, args);

   if (!SUCCEEDED(hr))
   {
      va_end(args);
      return;
   }

   szBuffer[cLargeBufferSize-1] = 0;

   if (crlf)
   {
      // MS 6/7/2005: we are okay with this not properly appending
      // the \r\n or \n in the edge case where we have a trace that
      // takes up 8k.
      StringCchCatA(szBuffer, countof(szBuffer), "\r\n");
   }

#ifndef BUILD_FINAL
   OutputDebugStringA(szBuffer);
#endif

   if (gTraceCallback)
      (*gTraceCallback)(szBuffer);
}

void setTraceCallback(BTraceCallback pCallback)
{
   gTraceCallback = pCallback;
}

