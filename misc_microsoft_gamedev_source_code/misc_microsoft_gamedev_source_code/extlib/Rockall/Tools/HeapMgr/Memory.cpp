#include "Memory.h"

extern HeapManager<ROCKALL_BLENDED_HEAP> gHeapMgr;

#undef new
#undef delete
#if USE_HEAP_MANAGER

void* __cdecl operator new(size_t size)
{
   return gHeapMgr.Allocate((long)size);
}

void __cdecl operator delete(void* data)
{
	// TODO: take the 'if' out
	if (data == NULL) {
      return;
	}
	gHeapMgr.Deallocate(data);
}


#if _MSC_VER >= 1300

void* __cdecl operator new[](size_t size)
{
   return gHeapMgr.Allocate((long)size);
}

void __cdecl operator delete[](void* data)
{ 
	// TODO: take the 'if' out
	if (data == NULL) {
		return;
	}
	gHeapMgr.Deallocate(data);
}

#endif // _MSC_VER

#endif // USE_HEAP_MANAGER

