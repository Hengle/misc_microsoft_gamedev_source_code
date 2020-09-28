#include "MemMgrTests.h"
#include "Memory.h"

int main (void)
{
	AllocMultipleDeallocMultipleTest();
	AllocateProperlyTest();
	DeallocateImproperlyTest();
	LeakMemoryTest(bfactor);
	AllocHugeMemoryBlockTest();
	ProperAllocDeallocTest();
	Overflow2GigLimitTest();
	ReallocHeapTest();
	// the call below is commented out because on exit 
	// a crash will occur after the heap is corrupted
	//CorruptHeapTest();
	DeleteAlreadyFreedMemoryTest();
	MemoryOnHeapTest();
	ValidateHeapTest();
	DeallocMemoryTest();
	DeallocHeapTest();

	return 0;
}