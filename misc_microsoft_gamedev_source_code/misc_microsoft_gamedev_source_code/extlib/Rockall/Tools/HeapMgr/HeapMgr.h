
#ifndef _HEAPMANAGER_H_
#define _HEAPMANAGER_H_

#define USE_HEAP_MANAGER	1

#define COMPILING_ROCKALL_LIBRARY

#ifdef COMPILING_ROCKALL_DLL
#define ROCKALL_DLL_LINKAGE	__declspec(dllexport)
#else
#ifdef COMPILING_ROCKALL_LIBRARY
#define ROCKALL_DLL_LINKAGE
#else
#define ROCKALL_DLL_LINKAGE	__declspec(dllimport)
#endif
#endif

#ifdef _DEBUG
#define USE_ROCKALL_DEBUG_HEAPS
//#define USE_PAGE_HEAP
#endif

// debug trace log will be @ c:\temp
// create this directory if it does not exist
#ifdef USE_ROCKALL_DEBUG_HEAPS
#ifdef USE_PAGE_HEAP
#define ROCKALL_FAST_HEAP		PAGE_HEAP
#define ROCKALL_SMALL_HEAP		PAGE_HEAP
#define ROCKALL_BLENDED_HEAP	PAGE_HEAP
#else // USE_PAGE_HEAP
#define ROCKALL_FAST_HEAP		DEBUG_HEAP
#define ROCKALL_SMALL_HEAP		DEBUG_HEAP
#define ROCKALL_BLENDED_HEAP	DEBUG_HEAP
#endif
#else  // USE_ROCKALL_DEBUG_HEAPS
#define ROCKALL_FAST_HEAP		FAST_HEAP
#define ROCKALL_SMALL_HEAP		SMALL_HEAP
#define ROCKALL_BLENDED_HEAP	BLENDED_HEAP
#endif

#include "Rockall/DebugHeap.hpp"
#include "Rockall/PageHeap.hpp"
#include "Rockall/SmallHeap.hpp"
#include "Rockall/FastHeap.hpp"
#include "Rockall/BlendedHeap.hpp"

#if USE_HEAP_MANAGER

template <typename TYPE>
class HeapManager
{
	TYPE heap;
  
	HeapManager(const HeapManager& alloc);
	const HeapManager& operator = (const HeapManager& alloc);

	// set the limit on the amount of unallocated memory the heap 
	// is allowed to cache before it is forced to return some to the operating system
	// signed MaxFreeSpace;
	
	// set to false, all deleted elements are passed through the heap caches and 
	// are eventually fully released before being reallocated
	// bool	Recycle;

	// set to true, allocations made from one heap and deleted on another heap
	// will be correctly deleted even though it has been supplied to the wrong heap for deletion
	// bool	SingleImage;

	// set to true, a heap will behave correctly even if multiple threads access it at the same time
	// bool	ThreadSafe;
#ifdef USE_ROCKALL_DEBUG_HEAPS
	// set to true, information about the top 5 functions on the call stack are stored in the
	// memory allocation and are output along with any message relating to the memory allocation 
	static const bool	FunctionTrace	= true;
	// set to false, tries to repair any damage in the heap, and does not raise an exception
	static const bool	TrapOnUserError	= true;
#endif // USE_ROCKALL_DEBUG_HEAPS

public:
	HeapManager(signed maxFreeSpace=(2*HalfMegabyte), bool recycle=false,
					bool singleImage=false, bool threadSafe=true);
	~HeapManager();
	void* Allocate (const long& size);
	void* Allocate (const long& sizeNeeded, long& sizeAllocated);

	// very fast method
	bool Deallocate (void* data, const long& size);
	// slow method, size of the data needs to be computed
	bool Deallocate (void* data);

	void* Reallocate (void* data, long& size, bool delMem=false, bool zeroMem=false);
	void DeallocateAll (bool recycle = false);
	bool IsAvailable (void);
	bool IsOnHeap (void* data);
	bool ValidateHeap (void);
	bool Flush (long sizeToFree=0);
	bool AllocateMultiple(int* elemsAllocated, void* data[], long neededElems,
					 long elemSize, int* actualElemSize=NULL, bool zero=false);
	bool DeallocateMultiple(long numElems, void* data[], int actualElemSize=NoSize);

protected:
	// you can change the scoping on these methods as needed
	bool Details (void* data, long& actualSize);
	bool Validate (void* data, long& actualSize);
	bool Walk (bool& active, void** data, long& actualSize);
};

template <typename TYPE>
HeapManager<TYPE>::HeapManager(signed maxFreeSpace, bool recycle,
					bool singleImage, bool threadSafe)
	:	heap(maxFreeSpace, 
		recycle, 
		singleImage, 
		threadSafe
#ifdef USE_ROCKALL_DEBUG_HEAPS
		, FunctionTrace
		, TrapOnUserError
#endif
	 )
{
}

template <typename TYPE>
HeapManager<TYPE>::~HeapManager()
{
#ifdef USE_ROCKALL_DEBUG_HEAPS
	heap.HeapLeaks();
#endif
}

// very fast method
// no size details need to be computed
// size of the memory block is provided
template <typename TYPE>
void* HeapManager<TYPE>::Allocate (const long& size)
{
	return heap.New(size);
}

// slow method, size of the data needs to be computed
template <typename TYPE>
void* HeapManager<TYPE>::Allocate (const long& sizeNeeded, long& sizeAllocated)
{
	return heap.New(sizeNeeded, (int*)&sizeAllocated);
}

template <typename TYPE>
bool HeapManager<TYPE>::Deallocate (void* data, const long& size)
{
	return heap.Delete(data, size);
}

template <typename TYPE>
bool HeapManager<TYPE>::Deallocate (void* data)
{
	int space = 0;

	if (!heap.Details(data, &space)) {
		// unable to compute the size of the block
		return false; 
	}

	if (!heap.Delete(data, space)) {
		return false;
	}

	return true;
}

// Deallocate multiple elements of the same size
// IN numElems: # of elements to be deleted
// IN data: pointer to the allocated array of elements, returned by 'Allocate' or 'AllocateMultiple'
// IN actualElemSizeAllocated: optional, size of the original allocations, 
// passed as an optimization to this function
// Very tolerant method, memory allocations of different sizes from different 
// allocation functions can be deleted together
template <typename TYPE>
bool HeapManager<TYPE>::DeallocateMultiple(long numElems, void* data[], int actualElemSize)
{
	return heap.MultipleDelete(numElems, data, actualElemSize);
}

// Allocate multiple elements of the same size
// OUT elemsAllocated: actual # of elements allocated
// OUT data: pointer to the allocated array of elements
// IN neededElems: # of elements to allocate
// IN elemSize: size of a single element
// OUT actualElemSizeAllocated: actual # of bytes allocated to each element
// IN zero: true, zero all new allocations, default is false
template <typename TYPE>
bool HeapManager<TYPE>::AllocateMultiple(int* elemsAllocated, void* data[], 
				 long neededElems, long elemSize, int* actualElemSize, bool zero)
{
	return heap.MultipleNew(elemsAllocated, data, neededElems, 
										elemSize, actualElemSize, zero);
}

// Flushes all the internal caches
// Releases the highest addressed blocks of free memory back to the operating system
// Space freed does not exceed the value of ‘sizeToFree’
template <typename TYPE>
bool HeapManager<TYPE>::Flush(long sizeToFree)
{
	return heap.Truncate(sizeToFree);
}

// Return value is true if all allocations on the heap are valid 
template <typename TYPE>
bool HeapManager<TYPE>::ValidateHeap(void)
{
	// walk the heap and validate every allocation
	bool isBad = false;
	bool active = false;
	void* data = NULL;
	int actualSize = 0;

	for ( ; ; ) {
		bool valid = true;
		// get the address of the memory block
		if (!heap.Walk(&active, &data, &actualSize)) {
			valid = false;
			break;
		}
		// verify the memory block is allocated 
		if (active) {
			valid = heap.Verify(data);
		}
		// if errors were reported
		if (!valid) {
			isBad = true;
		}
	}
	return !isBad;
}

// Returns true if the memory location is managed by the current heap 
// Returns true if the memory location is managed by some associated heap, 'SingleImage’ member must be true
template <typename TYPE>
bool HeapManager<TYPE>::IsOnHeap(void* data)
{
	return heap.KnownArea(data);
}

// If the heap is corrupt or unavailable, false is returned
template <typename TYPE>
bool HeapManager<TYPE>::IsAvailable(void)
{
	return heap.IsAvailable();
}

// OUT active: true if the information returned relates to allocated memory
// IN data: should point to a ‘NULL’ value
// OUT data: address of the memory block
// OUT actualSize: size of the memory block in bytes
// Return value is true if memory block is walked
template <typename TYPE>
bool HeapManager<TYPE>::Walk(bool& active, void** data, long& actualSize)
{
	return heap.Walk(active, data, actualSize);
}

// Same as GetDetails but 2-3 times slower
// IN data: pointer to the memory block
// OUT actualSize: size requested by the caller + the header overhead
// Return value is true if the data is currently allocated 
template <typename TYPE>
bool HeapManager<TYPE>::Validate(void* data, long& actualSize)
{
	return heap.Verify(data, actualSize);
}

// IN data: pointer to the memory block
// OUT actualSize: size requested by the caller + the header overhead
// Return value is true if the memory managed by the heap
template <typename TYPE>
bool HeapManager<TYPE>::Details (void* data, long& actualSize)
{
	return heap.Details(data, actualSize);
}

// IN recycle: true, memory is not returned to the operating system
// IN recycle: default is false, memory is returned to the operating system
template <typename TYPE>
void HeapManager<TYPE>::DeallocateAll(bool recycle)
{
	heap.DeleteAll(recycle);
}

// IN size: the size needed for the allocation
// OUT size: the actual size allocated
// del_mem: false, prevent the existing allocation from being deleted, this might
// need to be true if non-trivial dtors of objects need to be invoked
// zero_mem: true, zero the expanded portion of an allocation when it is resized
template <typename TYPE>
void* HeapManager<TYPE>::Reallocate (void* data, long& size, bool delMem, bool zeroMem)
{
	int		allocatedSize = 0;
	void*	newData = NULL;
	newData = heap.Resize(data, size, 1, &allocatedSize, delMem, zeroMem);

	size = allocatedSize;
	return newData;
}

#endif // USE_HEAP_MANAGER

#endif