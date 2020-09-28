//==============================================================================
// bsnprintf.cpp
//
// Copyright (c) 2000-2005, Ensemble Studios
//==============================================================================
#include "xcore.h"

// MS 6/9/2005: new versions of the safe sprintf functions
// that use strsafe string handling routines underneath.

// Smart _snprintf that terminates the string.  Like sprintf but takes a maximum number of
// chars to write.  Returns -1 if it can't finish writing, otherwise it returns the number
// of chars written minus the null terminator.
int bsnprintf(char *buff, size_t n, const char *format, ...)
{
   // Do the sprintf with limited size.
   va_list ap;
   va_start(ap, format);
   long length = -1;
   size_t numRemaining = 0;
   char* destEnd = NULL;
   HRESULT hr = StrHelperStringCchVPrintfExA(buff, n, &destEnd, &numRemaining, 0, format, ap);
   if(SUCCEEDED(hr))
   {
      length = destEnd - buff;
      BASSERT(length + numRemaining == n);
   }
   va_end(ap);

   // Return the length we wrote.
   return(length);
}

// Smart _vsnprintf that terminates the string.  Like vsprintf but takes a maximum number of
// chars to write.  Returns -1 if it can't finish writing, otherwise it returns the number
// of chars written minus the null terminator.
int bvsnprintf(char *buff, size_t n, const char *format, va_list argptr)
{
   // Do the sprintf with limited size.
   long length = -1;
   size_t numRemaining = 0;
   char* destEnd = NULL;
   HRESULT hr = StrHelperStringCchVPrintfExA(buff, n, &destEnd, &numRemaining, 0, format, argptr);
   if(SUCCEEDED(hr))
   {
      length = destEnd - buff;
      BASSERT(length + numRemaining == n);
   }

   // Return the length we wrote.
   return(length);
}

// Smart _snprintf that terminates the string.  Like sprintf but takes a maximum number of
// chars to write.  Returns -1 if it can't finish writing, otherwise it returns the number
// of chars written minus the null terminator.
int bsnbprintf(BCHAR_T *buff, size_t n, const BCHAR_T *format, ...)
{
   // Do the sprintf with limited size.
   va_list ap;
   va_start(ap, format);
   long length = -1;
   size_t numRemaining = 0;
   BCHAR_T* destEnd = NULL;
   HRESULT hr = StringCchVPrintfEx(buff, n, &destEnd, &numRemaining, 0, format, ap);
   if(SUCCEEDED(hr))
   {
      length = destEnd - buff;
      BASSERT(length + numRemaining == n);
   }
   va_end(ap);

   // Return the length we wrote.
   return(length);
}

// Smart _snprintf that terminates the string.  Like sprintf but takes a maximum number of
// chars to write.  Returns -1 if it can't finish writing, otherwise it returns the number
// of chars written minus the null terminator.
int bsnwprintf(WCHAR *buff, size_t n, const WCHAR *format, ...)
{
   // Do the sprintf with limited size.
   va_list ap;
   va_start(ap, format);
   long length = -1;
   size_t numRemaining = 0;
   WCHAR* destEnd = NULL;
   HRESULT hr = StringCchVPrintfExW(buff, n, &destEnd, &numRemaining, 0, format, ap);
   if(SUCCEEDED(hr))
   {
      length = destEnd - buff;
      BASSERT(length + numRemaining == n);
   }
   va_end(ap);

   // Return the length we wrote.
   return(length);
}

// Smart _vsnprintf that terminates the string.  Like vsprintf but takes a maximum number of
// chars to write.  Returns -1 if it can't finish writing, otherwise it returns the number
// of chars written minus the null terminator.
int bvsnwprintf(WCHAR *buff, size_t n, const WCHAR *format, va_list argptr)
{
   // Do the sprintf with limited size.
   long length = -1;
   size_t numRemaining = 0;
   WCHAR* destEnd = NULL;
   HRESULT hr = StringCchVPrintfExW(buff, n, &destEnd, &numRemaining, 0, format, argptr);
   if(SUCCEEDED(hr))
   {
      length = destEnd - buff;
      BASSERT(length + numRemaining == n);
   }

   // Return the length we wrote.
   return(length);
}

