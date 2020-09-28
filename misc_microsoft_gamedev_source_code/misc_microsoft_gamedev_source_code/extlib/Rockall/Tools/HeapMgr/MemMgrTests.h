#include "HeapMgr.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef unsigned char	byte;

const unsigned ptr_size = 10;
const unsigned kbyte = 1024;
const unsigned bfactor = 5;

struct SomeObject
{
	SomeObject() {}
	unsigned m_i;
	unsigned m_j;
};

// tests
void AllocMultipleDeallocMultipleTest(void);
void AllocateProperlyTest(void);
void DeallocateImproperlyTest(void);
void LeakMemoryTest(int bfactor);
void AllocHugeMemoryBlockTest(void);
void ProperAllocDeallocTest(void);
void Overflow2GigLimitTest(void);
void ReallocHeapTest(void);
void CorruptHeapTest(void);
void DeleteAlreadyFreedMemoryTest(void);
void MemoryOnHeapTest(void);
void ValidateHeapTest(void);
void DeallocMemoryTest(void);
void DeallocHeapTest(void);