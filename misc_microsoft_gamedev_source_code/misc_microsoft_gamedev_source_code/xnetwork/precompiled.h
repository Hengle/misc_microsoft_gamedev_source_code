
#pragma once

#ifndef PRECOMPILED_H
#define PRECOMPILED_H

#include <xsystem.h>

#ifndef XBOX
   #include <rpc.h>
#endif   

#include "winsockinc.h"

#include <process.h>

#include <etl/List.hpp>

#pragma warning (disable: 4786)      // identifier truncated to 'nnn' characters (usually caused by templates)
#pragma warning (disable: 4710)        // "Not inlined warning"
#pragma warning (disable: 4702)        // "Unreachable code warning"

//
// Whoever implements DllMain must provide and set this value.
//

extern HINSTANCE DllInstance;

static inline HRESULT GetLastResult (void)
{
   return HRESULT_FROM_WIN32 (GetLastError());
}

#include "Socket.h"
#include "Listener.h"
#include "Packet.h"
#include "commlog.h"
#include "commheaders.h"

#pragma warning ( disable : 4786 ) // warning C4786: identifier was truncated to '255' characters in the debug information
#pragma warning ( disable : 4710 ) // warning C4710 not inlined

#define WIN32_LEAN_AND_MEAN
//#define _WIN32_WINNT 0x400

#ifndef countof
#define countof(a) (sizeof(a) / sizeof(*a))
#endif

#ifndef RETURN_ERROR_IF_NULL
#define RETURN_ERROR_IF_NULL(ptr, text) \
{ \
   ; \
   if (!ptr) \
   { \
      nlog(cTransportNL, text); \
      return E_POINTER; \
   } \
} 
#endif


#endif // PRECOMPILED_H