#pragma once
#include "HeapUsageEntry.h"

// Empirical data on heap usage derived from Rockall logs.

class CHeapUsage
{

private:

public:
	int m_nSize;
	int m_nElementsUsed;

	CHeapUsageEntry *m_pHUE;
	int m_nBaseAddress;

	CHeapUsage(void);
	bool SetSize(int size);
	bool AddElement(int nRequest, int nSupplied, int nCount);
	void Reset(void);
public:
	~CHeapUsage(void);
};
