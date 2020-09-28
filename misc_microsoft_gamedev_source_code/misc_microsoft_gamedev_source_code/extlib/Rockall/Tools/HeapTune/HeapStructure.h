
#include "HeapStructureElement.h"
#include "HeapUsage.h"

#pragma once

/*
representation of a heap bucket size sequence
v1: superset of regularized FastHeap, new Stride sizes, bucket count penalty not included
todo: 
generalized stride count
cache line overhead
cache search overhead
non-increasing bucketsize delta
page packing efficiency
*/

class CHeapStructure
{
private:

public:
	int m_nLogStride1;
	int m_nLogStride2;

	// todo: this is a better representation - deprecate the other
	int m_nLogBucketSizeMin;
	int *m_rgnStructureLogBucketSizeStart;

	int m_nSize;
	int m_nUsed;
	int m_nMaxBinSize;

	bool SetSize(int nSize);
	void SetMax(int nSize);
	bool Add(int nLogDelta, int nStart);
	bool AddLinear(int nDelta, int nStart);
	CHeapStructureElement *m_rgHSE;
	bool SetStructure(int *rgnStructure);
	void GenerateHeap(int **prgnBinSize, int *pnBinCount);
	void GenerateHeap(int **prgnBinSize, int *pnBinCount, CHeapUsage *pUsage);
	void GenerateHeapFromArray(int **prgnBinSize, int *pnBinCount);
	void GenerateHeapFromArray(int **prgnBinSize, int *pnBinCount, int **prgnStructureKey);

	CHeapStructure(void);
	~CHeapStructure(void);
};
