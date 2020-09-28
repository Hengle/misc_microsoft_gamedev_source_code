// vc6mem.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "vc6mem.h"

#include <stdlib.h>

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
  switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

VC6MEM_API void* __cdecl VC6Mem_new( unsigned int s )
{
  // EVIL HACK
  return malloc(s);
}

VC6MEM_API void __cdecl VC6Mem_delete(void* p)
{
  // EVIL HACK
  free(p);
}

