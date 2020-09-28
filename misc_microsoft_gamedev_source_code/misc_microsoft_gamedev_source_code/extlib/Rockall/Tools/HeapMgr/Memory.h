#pragma once 

#include "HeapMgr.h"

#if USE_HEAP_MANAGER

void* __cdecl operator new(size_t size);
void __cdecl operator delete(void* data);

#if _MSC_VER >= 1300
void* __cdecl operator new[](size_t size);
void __cdecl operator delete[](void* pData);
#endif // _MSC_VER

#endif // USE_HEAP_MANAGER

