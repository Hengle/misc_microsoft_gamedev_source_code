//==============================================================================
// bsnprintf.h
//
// Copyright (c) 2000 - 2005, Ensemble Studios
//==============================================================================
#pragma once

// MS 6/9/2005: new versions of the safe sprintf functions
// that use strsafe string handling routines underneath.
// The 'b' is prepended so that strsafe.h doesn't complain.

// Smart _snprintf that terminates the string.  Like sprintf but takes a maximum number of
// chars to write.  Returns -1 if it can't finish writing, otherwise it returns the number
// of chars written minus the null terminator.
int bsnprintf(char *buff, size_t n, const char *format, ...);
int bsnwprintf(WCHAR *buff, size_t n, const WCHAR *format, ...);
int bsnbprintf(BCHAR_T *buff, size_t n, const BCHAR_T *format, ...);

// Smart _vsnprintf that terminates the string.  Like vsprintf but takes a maximum number of
// chars to write.  Returns -1 if it can't finish writing, otherwise it returns the number
// of chars written minus the null terminator.
int bvsnprintf(char *buff, size_t n, const char *format, va_list argptr);
int bvsnwprintf(WCHAR* buff, size_t n, const WCHAR *format, va_list argptr);
inline int bvsnbprintf(BCHAR_T *buff, size_t n, const BCHAR_T *format, va_list argptr)
{
#ifdef UNICODE
   return bvsnwprintf(buff, n, format, argptr);
#else
   return bvsnprintf(buff, n, format, argptr);
#endif   
}

//==============================================================================
// eof: bsnprintf.h
//==============================================================================
