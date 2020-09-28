//==============================================================================
// radixsorter.h
//
// optimizations based on "Radix Sort Revisited" by Pierre Terdiman
// http://codercorner.com/RadixSortRevisited.htm
// Copyright (c) 2002, Ensemble Studios
//==============================================================================
#pragma once

class BRadixSorter  
{
public:
	BRadixSorter(BMemoryHeap* pHeap = &gPrimaryHeap);
	~BRadixSorter();
		
	BRadixSorter&		sort(const float* input, unsigned long num);
	unsigned long*		getIndices(void) const { return mIndices; }
	
	void              freeIndices(void);

private:
   BRadixSorter(const BRadixSorter&);
   BRadixSorter& operator=(const BRadixSorter&);
   
	//-- local memory for histogram
	unsigned long		mHistogram[1024];
	unsigned long		mOffset[256];

	//-- indices storage
	unsigned long		mSize;
	unsigned long*    mIndices;
	unsigned long*    mIndices2;

   BMemoryHeap*      mpHeap;

	//-- private functions
	void				resize(long num);
	void				resetIndices( void);
	bool				createHistograms( const float *values, long num);
};
