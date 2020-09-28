#include "MemMgrTests.h"

HeapManager<ROCKALL_BLENDED_HEAP> gHeapMgr;
HeapManager<ROCKALL_FAST_HEAP> gFastHeap;

void AllocMultipleDeallocMultipleTest(void)
{
	HeapManager<ROCKALL_SMALL_HEAP> blendedHeap;

	// allocate multiple elements of SomeObject
	int elemAllocated = 0;
	const long neededElems = 10;
	long elemSize = sizeof(SomeObject);
	int actualElemSize = 0;
	SomeObject *pArray[neededElems];

	bool isGood = blendedHeap.AllocateMultiple(&elemAllocated, (void**)pArray, neededElems,
		elemSize, &actualElemSize);
	long deallocElems = 10;
	isGood = blendedHeap.DeallocateMultiple(deallocElems, (void**)pArray, actualElemSize);

	isGood = blendedHeap.AllocateMultiple(&elemAllocated, (void**)pArray, neededElems,
		elemSize, &actualElemSize, true);

	deallocElems = 5;
	isGood = blendedHeap.DeallocateMultiple(deallocElems, (void**)pArray, actualElemSize);
}

void AllocateProperlyTest(void)
{
	byte* p[ptr_size] = {0};

	p[1] = new byte;
	p[2] = new byte [1024];

	delete p[1];
	delete [] p[2];
}

void DeallocateImproperlyTest(void)
{
	byte* p[ptr_size] = {0};

	p[1] = new byte;
	p[2] = new byte [1024];

	delete [] p[1];
	delete p[2];

	p[3] = (byte*)987654321;
	p[4] = (byte*)987654321;

	delete p[3];
	delete [] p[4];
}

void LeakMemoryTest(int bfactor)
{
	void* data = gFastHeap.Allocate(bfactor*kbyte);
	(void)data;
}

void AllocHugeMemoryBlockTest(void)
{
	byte* p[ptr_size] = {0};

	p[5] = new byte [1*kbyte*kbyte];
	for(unsigned i =0; i < 1*kbyte*kbyte; i+=256) {
		p[5][i] = 0;
	}

	delete [] p[5];
}

void ProperAllocDeallocTest(void)
{
	byte* q = new byte;
	*q=65;

	delete q;
}

void Overflow2GigLimitTest(void)
{
	byte* p[ptr_size] = {0};
	// overflow to 2GB on 32bit
	long size = (unsigned)-1;
	try {
		p[5] = (byte*)gFastHeap.Allocate(size); 
	}
	catch (...) {
		OutputDebugString("Can't Allocate beyond 2Gig");
	}
}

void ReallocHeapTest(void)
{
	byte* p[ptr_size] = {0};
	p[5] = (byte*)gFastHeap.Allocate(kbyte);

	long newSize=0;
	gFastHeap.Reallocate(p[5], newSize);
}

void CorruptHeapTest(void)
{
	HeapManager<ROCKALL_SMALL_HEAP> smallHeap;
	int* t = (int*)smallHeap.Allocate(kbyte);
	// corrupt the heap
	for (int i=-2; i < 0; i++) {
		t[i] = 0xFFFFFFFF;
	}

	bool isOK = true;
	try {
		isOK = smallHeap.ValidateHeap();
	}
	catch (...) {
		OutputDebugString("Corrupted Heap");
	}

	try	{
		smallHeap.Deallocate(t);
	}
	catch(...) {
		OutputDebugString("Could not free corrupted heap");
	}
}

void DeleteAlreadyFreedMemoryTest(void)
{
	byte* p[ptr_size] = {0};
	// delete already deleted memory
	p[7] = new byte;
	p[8] = new byte[kbyte];
	delete [] p[8];
	delete p[7];

	try {
		delete p[7];
		delete [] p[8];
	}
	catch (...) {
		OutputDebugString("Freeing Memory already been Freed");
	}
}

void MemoryOnHeapTest(void)
{
	byte* p[ptr_size] = {0};

	p[4] = (byte*)gHeapMgr.Allocate(kbyte);
	p[5] = (byte*)gFastHeap.Allocate(kbyte);

	// should not be on heap
	bool onHeap = gFastHeap.IsOnHeap(p[4]);
	// should be on heap
	onHeap = gFastHeap.IsOnHeap(p[5]);

	(void) onHeap;
}

void ValidateHeapTest(void)
{
	bool isOK = false;
	// validate the global heap
	isOK = gHeapMgr.ValidateHeap();
}

void DeallocMemoryTest(void)
{
	byte* p[ptr_size] = {0};
	p[5] = (byte*)gFastHeap.Allocate(kbyte);
	
	long newSize=0;
	gFastHeap.Reallocate(p[5], newSize);

	// already resized above to 0 
	// checkFree should be false, no memory to free
	bool checkFree = gFastHeap.Deallocate(p[5]);

	p[5] = (byte*)gFastHeap.Allocate(kbyte);
	// checkFree should be true, there is memory to free
	checkFree = gFastHeap.Deallocate(p[5]);
	(void) checkFree;
}

void DeallocHeapTest(void)
{
	byte* p[ptr_size] = {0};
	HeapManager<ROCKALL_SMALL_HEAP> smallHeap;
	p[0] = (byte*)smallHeap.Allocate(kbyte);

	smallHeap.DeallocateAll();
}
