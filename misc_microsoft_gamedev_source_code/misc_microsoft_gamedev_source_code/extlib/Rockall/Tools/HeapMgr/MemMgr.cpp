// MemMgr.cpp : Defines the entry point for the console application.
//

#include "HeapMgr.h"
#include "Memory.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>



//#define _SHOW_BUG


int main()
{
	



	

	
	
	



	

	// leak memory
	p[9] = (byte*)gFastHeap.Allocate(kbyte*kbyte);
	LeakMemoryIntentionally(3);


	

	return 0;
}

