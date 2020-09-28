//============================================================================
//
// File: setThreadName.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"

// rg [2/16/06] - Copied from XDK docs.

#ifdef XBOX
typedef struct tagTHREADNAME_INFO {
   DWORD dwType;     // Must be 0x1000
   LPCSTR szName;    // Pointer to name (in user address space)
   DWORD dwThreadID; // Thread ID (-1 for caller thread)
   DWORD dwFlags;    // Reserved for future use; must be zero
} THREADNAME_INFO;

//============================================================================
// SetThreadName
//============================================================================
void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
   THREADNAME_INFO info;

   info.dwType = 0x1000;
   info.szName = szThreadName;
   info.dwThreadID = dwThreadID;
   info.dwFlags = 0;

   __try
   {
      RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD *)&info );
   }
   __except( EXCEPTION_CONTINUE_EXECUTION )
   {
   }
}
#else // !XBOX
//============================================================================
// SetThreadName
//============================================================================
void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
   dwThreadID;
   szThreadName;
}
#endif // XBOX