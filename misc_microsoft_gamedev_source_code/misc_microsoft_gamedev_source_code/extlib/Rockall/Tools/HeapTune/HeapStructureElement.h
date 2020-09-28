#pragma once

// One element of the "heap structure" array that defines a heap.  
// Note that values in this array are not bucket sizes, but instead are bucket size deltas.

class CHeapStructureElement
{
public:

	int m_nLogDelta;
	int m_nDelta;
//	int m_nCount;
	int m_nStart;
	bool Set(int nLogDelta, int nStart);
	bool SetLinear(int nDelta, int nStart);

	CHeapStructureElement(int nLogDelta, int nStart);
	CHeapStructureElement(void);
	~CHeapStructureElement(void);
};
