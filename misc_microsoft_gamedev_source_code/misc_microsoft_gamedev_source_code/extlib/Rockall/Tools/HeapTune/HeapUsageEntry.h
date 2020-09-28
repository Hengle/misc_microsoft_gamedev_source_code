#pragma once


// One element of the list representing empirical heap usage.

class CHeapUsageEntry
{

public:

	int m_nRequestedSize;
	int m_nSuppliedSize;
	int m_nCallCount;

	CHeapUsageEntry(int nRequest, int nSupplied, int nCount);
	CHeapUsageEntry();
	void Set(int nRequest, int nSupplied, int nCount);

	~CHeapUsageEntry(void);
};
